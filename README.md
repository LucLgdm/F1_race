# F1_race

A top-down racing simulation written in modern C++20 with OpenGL. The player drives with the keyboard; the long-term goal is to add opponents controlled by a neural network implemented from scratch.

This is a learning project: build a working simulation, understand neural-network inference, and then implement training before connecting the model to a car.

## Current progress

- GLFW window and OpenGL rendering using GLAD.
- A car with acceleration, braking, steering, and visible wheels.
- A fixed simulation timestep of `1 / 120` second.
- A camera following the player without rotating with the car.
- A configurable rectangular track.
- Collision checks that restore the previous car state and stop it.
- Neural-network forward passes through neurons, layers, and networks.
- Configurable Linear, ReLU, and Tanh activations.
- Mean squared error to evaluate predictions.

Training, driving sensors, recorded demonstrations, and autonomous opponents are not implemented yet. Circuit shapes will be developed separately.

## Requirements and build

The development environment is Windows with MSYS2 UCRT64. The project uses:

| Dependency | Purpose |
| --- | --- |
| C++20 compiler | Application and ML code, including `std::span` |
| C compiler | Compile `src/glad.c` |
| CMake 3.20 or later | Build configuration |
| Ninja | Build tool for the commands below |
| GLFW | Window, input, and OpenGL context |
| OpenGL | Rendering |
| GLAD | Load OpenGL functions |
| GLM | Vectors and transformation matrices |

GLAD and GLM headers are present under `include/`. GLFW must be discoverable by CMake. The CMake project enables both languages:

```cmake
project(F1_race LANGUAGES C CXX)
```

From the project root, using the MSYS2 UCRT64 environment:

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/F1_race.exe
```

Run the executable from the project root because shader paths such as `shaders/vertex.glsl` are relative to the working directory. Include the implementation files for `Track`, `Renderer`, and the ML classes in the executable's CMake source list. Headers alone do not compile their corresponding `.cpp` files automatically.

## Controls

| Key | Action |
| --- | --- |
| W | Accelerate |
| S | Brake |
| A | Steer left |
| D | Steer right |
| Escape | Request window closure |

Forward motion is currently supported; reverse gear is not implemented. The window-close request uses `glfwSetWindowShouldClose()`, while `glfwWindowShouldClose()` reads that flag.

## Project layout

```text
F1_race/
├── CMakeLists.txt
├── README.md
├── note.txt
├── include/
│   ├── Application.hpp
│   ├── Car.hpp
│   ├── Exception.hpp
│   ├── Renderer.hpp
│   ├── Track.hpp
│   ├── Vec2.hpp
│   ├── ML/
│   │   ├── Activation.hpp
│   │   ├── Layer.hpp
│   │   ├── Loss.hpp
│   │   ├── Network.hpp
│   │   └── Neuron.hpp
│   ├── KHR/
│   ├── glad/
│   └── glm/
├── shaders/
│   ├── vertex.glsl
│   └── fragment.glsl
└── src/
    ├── Application.cpp
    ├── Car.cpp
    ├── Renderer.cpp
    ├── Track.cpp
    ├── glad.c
    ├── main.cpp
    └── ML/
        ├── Layer.cpp
        ├── Network.cpp
        ├── Neuron.cpp
        └── testML.cpp
```

`Exception.hpp` and `note.txt` are also present; they are not part of the core simulation/ML separation described below. `testML.cpp` contains the project's ML demonstration/checking code, including examples such as `testNetwork()`.

## Architecture

### Simulation and rendering

| Component | Responsibility |
| --- | --- |
| `Application` | Own the application lifecycle, process input, advance the simulation, coordinate collisions, and request rendering |
| `Car` | Store position, heading, speed, and steering angle; update motion from `CarControls` |
| `Track` | Construct the road geometry, provide the starting position, and determine whether the car's collision circle is on the road |
| `Renderer` | Manage OpenGL resources, configure the camera, and draw the track and cars |
| `Vec2` | Shared two-dimensional coordinates independent of OpenGL |

`Car` and `Track` describe the simulated world. They do not issue OpenGL commands. `Renderer` draws their state without deciding how cars move. The ML code operates on numbers and remains independent of these classes.

The frame loop follows this order:

1. Poll events and read keyboard controls.
2. Accumulate elapsed time.
3. Advance physics in fixed steps. For each step, save the car state, update it, and check the candidate position against the track.
4. Restore the previous state and set speed to zero if a collision occurs.
5. Clear the framebuffer, configure the view, draw the track, draw the car, and swap buffers.

The renderer's public drawing sequence is:

```cpp
renderer.beginFrame(car.position(), width, height);
renderer.drawTrack(track);
renderer.drawCar(car);
```

The track is drawn once per frame. Additional cars will be drawn using the same view and projection.

### Coordinates, camera, and vehicle motion

Simulation distances are expressed in metres, heading in radians, and speed in metres per second. Positive Y points upward. A car with zero heading faces positive X.

The vertex shader transforms each local vertex as follows:

```glsl
gl_Position = projection * view * model * vec4(position, 0.0, 1.0);
```

| Matrix | Responsibility |
| --- | --- |
| `model` | Place and orient one object in the world |
| `view` | Translate the world by the negative camera position |
| `projection` | Map the visible world region to clip coordinates using an orthographic projection |

The camera follows the car's position but keeps a fixed orientation. Changing `halfHeight` changes the visible area, not the physical track dimensions. The visible width is adjusted to the framebuffer aspect ratio.

Steering uses a simple kinematic bicycle model:

$$
\dot{\theta}=\frac{v}{L}\tan(\delta)
$$

Here, $v$ is speed, $L$ is wheelbase, and $\delta$ is the front-wheel steering angle. The car cannot rotate in place at zero speed. This model does not yet simulate tyre grip or sliding.

### Collision approximation

The car is represented by a circle of radius 2.3 metres. `Track::canDrive()` checks that the circle remains within the outer track boundary and does not intersect the forbidden central rectangle.

This is discrete collision detection: it checks the candidate position after each physics step. It can miss an obstacle crossed entirely between two positions. A continuous collision method would inspect the swept motion and find the first time of contact. The small fixed timestep reduces this risk but does not eliminate it in general.

The collision circle is conservative relative to the drawn car, so contact can happen before a visible part reaches a wall. Accelerating into a wall leaves the car blocked; recovery behaviour is future work.

### Resource lifetime

OpenGL resource creation occurs after the context is made current and GLAD is loaded. Renderer cleanup occurs before destroying that context. GPU buffers and shader programs require explicit OpenGL deletion; the current design uses `Renderer::shutdown()` to control that ordering.

## Machine learning introduction

In ordinary programming, the developer writes the decision rules. In machine learning, a model contains adjustable parameters, and training searches for values that produce useful predictions or actions.

For a driving bot, observations could include distances to track boundaries and vehicle speed. Outputs could represent steering, acceleration, and braking. The model transforms numerical observations into numerical commands.

| Learning approach | Principle | Connection to this project |
| --- | --- | --- |
| Supervised learning | Learn from inputs paired with expected outputs | Imitate recorded player commands |
| Unsupervised learning | Find structure in data without target answers | Not needed for our initial driving model |
| Reinforcement learning | Learn actions through interaction and cumulative rewards | Later, reward progress and penalize collisions |

Our initial plan is supervised imitation. Record observations and player commands, train on these pairs, and use the trained network to produce controls. Reinforcement learning can come later; rewards must encourage progress along the track, rather than speed alone.

## Neural-network implementation

### Class responsibilities

| Component | Responsibility |
| --- | --- |
| `Neuron` | Own weights, a bias, and an activation; compute one output |
| `Layer` | Own several neurons and return their outputs |
| `Network` | Own layers and pass each layer's outputs to the next |
| `Activation.hpp` | Define Linear, ReLU, and Tanh and apply them |
| `Loss.hpp` | Compute mean squared error |

`std::vector` owns parameters and intermediate results. `std::span<const float>` provides read-only access to inputs without owning or copying them. Its referenced storage must remain valid during the call. The classes use value ownership; smart pointers and inheritance are unnecessary for the current design.

### One neuron

A neuron computes a weighted sum with a bias, then applies an activation:

$$
z=\sum_{i=1}^{n}w_i x_i+b,
\qquad \hat y=f(z)
$$

- $x_i$: inputs.
- $w_i$: weights, one per input.
- $b$: bias, added independently of the inputs.
- $f$: activation function.
- $\hat y$: predicted output.

For inputs `{0.5, 0.8}`, weights `{0.4, -0.2}`, and bias `0.1`:

$$
z=0.4(0.5)-0.2(0.8)+0.1=0.14
$$

With ReLU, the output remains `0.14`. If the weighted sum is negative, ReLU returns zero.

### Activations

| Function | Formula | Output range |
| --- | --- | --- |
| Linear | $f(z)=z$ | Unrestricted |
| ReLU | $f(z)=\max(0,z)$ | Nonnegative |
| Tanh | $f(z)=\tanh(z)$ | Between -1 and +1 |

ReLU and Tanh introduce nonlinearity. Without nonlinear activations, stacking affine layers still produces an affine function. Tanh is suitable for a signed steering output; acceleration and braking will need an appropriate output interpretation as well. The current neuron constructor defaults to ReLU.

### Layers and forward propagation

In a dense layer, every neuron receives the same input vector, but has its own weights and bias. Three neurons produce three output values.

The example network has two inputs, a first layer of three neurons, and a final layer of one neuron. The final neuron therefore needs three weights.

```cpp
std::vector<float> outputs = _layers.front().forward(inputs);

for (std::size_t i = 1; i < _layers.size(); ++i)
{
    outputs = _layers[i].forward(outputs);
}
```

`front()` accesses the first layer. Its output becomes the next layer's input. The loop begins at index 1 because layer 0 has already been evaluated. The right-hand call finishes before its result replaces `outputs`.

For our manual example, the first layer returns approximately `{0.14, 0.33, 0.0}`. The final neuron uses weights `{0.5, -0.4, 0.2}` and bias `0.1`:

$$
\hat y=\max(0,\;0.5\times0.14-0.4\times0.33+0.2\times0+0.1)=0.038
$$

This is inference, or a forward pass. It uses the current parameters without changing them.

### Loss: mean squared error

Mean squared error compares predictions with expected outputs:

$$
L=\frac{1}{N}\sum_{i=1}^{N}(\hat y_i-y_i)^2
$$

Each prediction requires one target. The target count matches the network's output count, not its input count. Here, the formula averages over the output values for one example; dataset evaluation additionally averages over examples.

| Predictions | Targets | Expected MSE |
| --- | --- | --- |
| `{0.2, 0.8}` | `{0.5, 0.6}` | `0.065` |
| `{0.038}` | `{0.5}` | `0.213444` |

Squaring prevents positive and negative errors from cancelling. Matching predictions and targets give zero loss. Empty or differently sized arrays are rejected by `meanSquaredError()`.

### Training: gradients and updates

Training is the next implementation step. For one linear neuron with one input:

$$
\hat y=wx+b,
\qquad L=(\hat y-y)^2
$$

The chain rule gives:

$$
\frac{\partial L}{\partial w}=2(\hat y-y)x,
\qquad
\frac{\partial L}{\partial b}=2(\hat y-y)
$$

Gradient descent updates the parameters using learning rate $\eta$:

$$
w\leftarrow w-\eta\frac{\partial L}{\partial w},
\qquad
b\leftarrow b-\eta\frac{\partial L}{\partial b}
$$

A gradient describes how the loss changes with a parameter. Subtracting a sufficiently small multiple of it moves toward lower loss locally; an excessively large learning rate can prevent convergence.

For `x = 2`, target `y = 4`, initial `w = 0`, `b = 0`, and learning rate `0.1`, the initial loss is `16`. The gradients are `-16` and `-8`, so one update gives `w = 1.6` and `b = 0.8`. The prediction becomes `4` for this example.

Fitting that single example does not establish the general rule `y = 2x`: many parameter pairs fit it. Training on varied examples and evaluating on held-out data are needed to assess generalization.

For nonlinear neurons, the gradient also includes the activation derivative. For multiple layers, backpropagation applies the chain rule backward through the network. Backpropagation calculates gradients; gradient descent uses those gradients to update parameters. Neither step is implemented yet.

## Checks while developing

The current ML examples should produce approximate values, because floating-point calculations need not match decimal arithmetic exactly:

| Example | Expected result |
| --- | --- |
| First ReLU neuron example | `0.14` |
| Three-neuron layer | `{0.14, 0.33, 0.0}` |
| Two-layer example network | `0.038` |
| Tanh with weighted sum `-0.5` | `-0.462117` |
| Two-output MSE example | `0.065` |

Input counts must match neuron weight counts, and each layer must produce the input count expected by the next. Compile and link status checks are also important for GLSL: an undeclared uniform can prevent shader compilation, while mismatched uniform names can prevent the C++ code from uploading the intended matrices.

## Roadmap

1. Train a single linear neuron on several simple examples.
2. Implement activation derivatives and backpropagation through layers.
3. Initialize weights appropriately and train a complete small network.
4. Add numerical observations, such as ray distances to track boundaries and speed.
5. Normalize observations and record player demonstrations with matching commands.
6. Separate training and validation data, train an imitation model, and evaluate it in the simulation.
7. Connect predictions to `CarControls`, persist learned parameters, and add opponents.
8. Improve recovery behaviours and consider reinforcement learning.

Imitation has a limitation: a bot's errors can put it into situations absent from the recorded demonstrations. Recovery examples and later training improvements will be important. Multi-car racing will also require observations of other cars; track-boundary sensors alone will not teach overtaking or avoidance.

The immediate goal is understanding the learning loop before integrating it with driving: predict, measure error, calculate gradients, and update parameters.
