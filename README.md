# F1_race

A top-down racing simulation written in modern C++20 with OpenGL. The player drives with the keyboard; the long-term goal is to add opponents controlled by a neural network implemented from scratch.

This is a learning project: build a working simulation, implement neural networks from scratch, and compare autonomous driving learned through neuroevolution and reinforcement learning. Supervised examples are used to understand and check gradient-based training, rather than as the main driving strategy.

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
- Random weight initialization, with biases initially set to zero.
- Activation derivatives and gradient-descent training for individual neurons.
- Backpropagation through complete dense networks, checked on a simple regression task.
- Five wall-distance sensors, verified on the rectangular track.
- Observation helpers for speed, track-relative heading, and eight normalized network inputs.

Autonomous driving training is not implemented yet. The next stage is to extract a rendering-independent simulation and train a population of 10–20 cars through selection and mutation. Reinforcement learning with backpropagation will be added later for comparison. Model persistence, track/model association, and training display options are planned. Circuit shapes will be developed separately.

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
│   ├── CarObservation.hpp
│   ├── CarSensors.hpp
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

`CarObservation.hpp` and `CarSensors.hpp` are the observation headers introduced in the latest development steps. `Exception.hpp` and `note.txt` are also present. `testML.cpp` contains the ML examples, including single-neuron and whole-network training. Planned classes such as `Simulation`, `Agent`, and the trainers are not yet part of this layout.

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

### Planned orchestration and training architecture

`Application` remains the orchestrator: select a circuit and training method, start or resume training, choose display options, and launch gameplay with a saved model. Physics and evaluation will be extracted from its current loop into a simulation that works without GLFW or OpenGL.

| Planned component | Responsibility |
| --- | --- |
| `Simulation` | Advance the physical world at a fixed timestep, expose observations, detect collisions, track progress, and reset episodes |
| `Agent` | Own one car, its network, its score, and its active/finished state |
| `EvolutionTrainer` | Evaluate a population, select parents, retain elites, and mutate offspring |
| `SupervisedTrainer` | Organize input/target examples using the existing gradient-based training methods |
| `ReinforcementTrainer` | Later, implement a reward-based algorithm with a suitable gradient objective |
| `Renderer` | Optionally draw the simulation, all agents or a selected agent, and sensor rays |

These names describe the intended responsibilities, not completed interfaces. World stepping and agent ownership will be arranged so that every active car advances exactly once per simulation tick and all agents share the same track geometry.

Training display is optional. Keep physics at `1 / 120` second; render every N simulation ticks, pause and advance one tick manually, or omit rendering entirely. Rendering less frequently must not increase the physical timestep or skip physics updates. Headless training need not wait for real time.

The initial population will have no car-to-car collisions: each car is an independent driving trial on the same circuit. Displaying 20 cars does not require 20 threads; they can be updated sequentially during each fixed tick.

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

The driving plan has changed from imitation to autonomous trial and evaluation. The first approach will be neuroevolution; the second will be reinforcement learning using backpropagation. Both will share observations, control meanings, physics, and evaluation rules. Imitation remains a possible optional baseline, but recording player demonstrations is no longer the next implementation step.

## Driving observations

Five rays originate at the car centre and rotate with its heading. `Track::raycast()` checks the outer rectangle and the forbidden inner rectangle, returning the first wall intersection within the configured range. Measurements are from the centre, not from the collision-circle boundary.

| Sensor | Offset from car heading |
| --- | --- |
| Left | +90 degrees |
| Front-left | +45 degrees |
| Front | 0 degrees |
| Front-right | -45 degrees |
| Right | -90 degrees |

The current range is 40 metres. If no wall is encountered within that range, the measurement is 40 metres. The car uses direction `{cos(heading), sin(heading)}`; zero heading faces +X and positive angles turn left. Keyboard steering remains `-1 = left`, `+1 = right`, converted to the corresponding wheel angle in `Car::update()`.

`CarObservation` contains the five distances, speed, and a signed heading error. `measureWallDistances()` measures the rays; `observeCar()` combines those measurements with the vehicle state and track reference heading. `normalized()` produces these eight inputs in a fixed order:

| Index | Input | Encoding |
| --- | --- | --- |
| 0 | Left wall distance | distance / sensor range |
| 1 | Front-left wall distance | distance / sensor range |
| 2 | Front wall distance | distance / sensor range |
| 3 | Front-right wall distance | distance / sensor range |
| 4 | Right wall distance | distance / sensor range |
| 5 | Speed | speed / 50 m/s |
| 6 | Heading error | sine of the signed error |
| 7 | Heading error | cosine of the signed error |

The two orientation values avoid the discontinuity between angles near -pi and +pi. The reference direction is currently defined by the nearest segment of the rectangular track centreline, traversed clockwise. The upper straight points toward +X. The signed error is `car.heading() - track.referenceHeading(position)`, wrapped with `std::remainder()`.

This reference heading changes abruptly at rectangular corners. A later track representation can provide a smoother tangent. It is an orientation observation, not yet a lap-progress or checkpoint implementation.

Observations are computed before the corresponding physics update:

```cpp
const auto observation = observeCar(car, track, 40.0f);
const auto inputs = observation.normalized(40.0f, 50.0f);
// A future controller will turn network outputs into CarControls.
car.update(controls, fixedDt);
```

For the 120 x 80 metre track with a 10 metre road, the initial centre position and zero heading give distances approximately `{5, 7.071, 40, 7.071, 5}`. At rest, normalized inputs are approximately `{0.125, 0.1768, 1, 0.1768, 0.125, 0, 0, 1}`. These five distance measurements have been checked.

Three driving outputs are planned: steering, throttle, and brake. Their decoding must preserve steering in `[-1, 1]` and throttle/brake in `[0, 1]`. Activation selection and any remapping/clamping must be explicit and consistent during both training and gameplay. The final driving architecture is not fixed; its input layer must accept eight values.

## Neural-network implementation

### Class responsibilities

| Component | Responsibility |
| --- | --- |
| `Neuron` | Own weights, a bias, and an activation; compute one output and backpropagate gradients |
| `Layer` | Own several neurons; compute outputs and sum their input-gradient contributions |
| `Network` | Own layers; run inference and supervised training through the full network |
| `Activation.hpp` | Define Linear, ReLU, and Tanh and their derivatives |
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

ReLU and Tanh introduce nonlinearity. Without nonlinear activations, stacking affine layers still produces an affine function. Tanh is suitable for a signed steering output; acceleration and braking will need an appropriate output interpretation as well. The manual-weight constructor introduced ReLU as its default; the random-weight constructor introduced Linear as its default. Specify the activation explicitly when assembling a network.

Random initialization currently samples each weight uniformly from `[-0.5, 0.5)` using `std::mt19937` seeded by `std::random_device`; biases start at zero. This is a simple initial scheme, not a fan-in-scaled initialization. Reproducible experiments will require explicit seed management.

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

Gradient-based training is implemented and has been exercised on single neurons and a complete network. For one linear neuron with one input:

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

For nonlinear neurons, the gradient also includes the activation derivative. The implemented derivatives are Linear: `1`, ReLU: `z > 0 ? 1 : 0`, and Tanh: `1 - tanh(z)^2`. ReLU uses a derivative convention of zero at the nondifferentiable point `z = 0`.

### What moves backwards between layers?

The forward pass calculates every intermediate neuron output. We know those actual values; we do not know individual ideal targets for hidden neurons. There is no additional target-comparison error calculated between hidden layers.

Instead, backpropagation passes the gradient of the final loss with respect to each intermediate value. For one neuron with output `a`, let `g` be the incoming gradient of the final loss with respect to `a`:

$$
\delta=g f'(z),\qquad
\frac{\partial L}{\partial w_i}=\delta x_i,\qquad
\frac{\partial L}{\partial b}=\delta,\qquad
\frac{\partial L}{\partial x_i}=\delta w_i
$$

The input gradients are sent to the preceding layer. If several neurons depend on the same input, their contributions are added. Input gradients must be calculated with the weights used for the forward pass, before those weights are updated.

### Current training methods

| Method | Current behaviour | Used by neuroevolution? |
| --- | --- | --- |
| `forward()` | Compute outputs without changing parameters | Yes; inference is identical for every learning method |
| `Neuron::backward()` | Calculate input gradients, update weights and bias, and return the input-gradient vector | No |
| `Layer::backward()` | Call each neuron and sum input gradients | No |
| `Network::train()` | Cache layer activations, calculate MSE gradients, traverse layers backwards, and return the pre-update loss | No |

The implementation combines gradient calculation and parameter updates in `backward()`. Conceptually, backpropagation computes gradients and the optimizer uses them to update parameters. A future reinforcement trainer may need those operations separated for its own objective and update schedule.

For MSE over N output values, the initial output gradient is:

$$
\frac{\partial L}{\partial \hat y_i}=\frac{2}{N}(\hat y_i-y_i)
$$

For an isolated neuron, `train()` supplies `2 * (prediction - target)` to `backward()`. Its returned input gradients can be discarded because there is no earlier trainable layer.

`trainSupervised()` is a proposed clearer name for the current `train()` method; the rename is not assumed to be applied yet. Keep one shared `forward()`, rather than creating `forwardRetro()` and `forwardEvo()`. Separate network instances hold separately learned parameter sets, even though they use the same class and forward calculation.

## Two autonomous learning approaches

Backpropagation is a gradient-computation mechanism, not a complete reward-based learning algorithm. The two planned autonomous approaches use different ways to obtain parameters, while sharing inference and simulation.

| Aspect | Neuroevolution: first implementation | Reinforcement learning: later comparison |
| --- | --- | --- |
| Experience | Complete driving trials from a population of networks | Interactions containing observations, actions, rewards, and episode endings |
| Learning signal | Fitness score per trial | Rewards and an algorithm-specific learning objective |
| Parameter changes | Parent selection, copying, and random mutations | Gradients calculated by backpropagation with a suitable optimizer |
| Desired command labels | Not required | Not required |
| Existing `forward()` | Reused | Reused |
| Existing MSE `train()` alone | Not used | Insufficient to implement reward-based driving |
| Status | Planned | Planned; no specific algorithm selected yet |

Supervised learning is already implemented for numerical exercises and could support an optional imitation baseline. It needs known targets. A lap score cannot simply be passed as the three desired driving commands to the existing `train()` method.

### Neuroevolution: population and generations

Start with 10–20 independent agents. Each has the same network architecture but initially random weights, its own car state, and its own score. All start under the same evaluation conditions on the selected track.

1. Run each active car using its network predictions.
2. End a trial on collision, timeout, or another configured terminal condition.
3. Evaluate the population when all trials end or the generation time limit is reached.
4. Preserve the best network unchanged: elitism.
5. Copy selected parent parameters into offspring and apply independent mutations.
6. Reset the cars and begin the next generation.

One proposed population of 20 is one unchanged elite, 17 independently mutated copies of the winner, and two newly randomized networks. This is a starting configuration, not implemented code or a guaranteed optimum. Selecting several parents can preserve diversity if a single-parent population stagnates.

A mutation adds a small random perturbation to a weight or bias, optionally only for a sampled subset of parameters:

$$
w_{\mathrm{child}}=w_{\mathrm{parent}}+\varepsilon
$$

Copies must own their parameter storage independently. One offspring's mutation must not change its parent or another agent. Value ownership in the existing vectors supports this when ordinary deep copies are used. Parameter access/mutation APIs remain to be implemented.

### Progress, rewards, and episode rules

The initial fitness should prioritize the furthest valid forward progress around the circuit. Once agents complete laps, completion and lap time should influence ranking. Count progress along the route, not total movement distance: driving circles or shuttling backwards and forwards must not accumulate artificial progress.

Ordered checkpoints and a validated finish-line crossing are planned to track direction and laps. The current `referenceHeading()` helper alone does not implement this. Collision penalties, maximum trial duration, and termination after prolonged lack of progress will bound trials.

A possible per-step reward for later reinforcement learning combines signed route progress, elapsed-time cost, collision penalty, and a validated lap-completion bonus. Exact coefficients and fitness ranking remain to be chosen. Rewarding survival alone can favour stopping; rewarding speed alone can favour crashing; rewarding only completed laps gives beginners little feedback.

### Comparing the approaches

Use the same track versions, observation encodings, control meanings, collision rules, and evaluation scenarios. Where applicable, use the same policy-network architecture. Evaluate completion rate, successful lap times, progress, total environment steps, and training wall-clock time with rendering disabled. Count the steps of every agent, not just the number of generations. Repeat experiments with recorded seeds and report failed trials as well as successful laps.

## Planned model persistence

The intended workflow is: choose or create a track, train without mandatory rendering, save the learned model, and load its associated model for gameplay. A model per track is supported by the design; multi-track training could be explored later.

A saved policy must include its architecture, activation choices, weights, biases, observation order, sensor range, normalization constants, output decoding, and compatible track identifier/version. Keep learning-method metadata and evaluation results with it. Resuming an entire evolutionary run also needs population and random-generator state; resuming reinforcement training may need optimizer and other algorithm state beyond the driving policy.

Example future organization (not files already created):

```text
circuits/
    rectangle.track
models/
    rectangle_evolution.network
    rectangle_reinforcement.network
```

Changing the track or observation format can invalidate an association or require retraining. Model loading, saving, and automatic track/model selection are not implemented yet.

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

Completed or introduced in the current learning steps:

- Neurons, dense layers, networks, configurable activations, and MSE.
- Random initialization and single-neuron gradient-descent training.
- Full-network backpropagation and numerical regression examples.
- Five verified wall-distance sensors, normalized observations, and a rectangular-track reference-heading helper.

Next implementation stages:

1. Extract physics, observations, collision reporting, and reset behaviour into a rendering-independent simulation.
2. Define network-output decoding into steering, throttle, and brake.
3. Add ordered checkpoints, forward progress, lap validation, timeouts, and trial scoring.
4. Add an agent population with independently owned cars and networks.
5. Implement parameter copying and mutation, elite preservation, and generation management.
6. Add training controls in `Application`: track selection, optional rendering, rendering interval, single-step mode, selected-agent tracking, and optional ray display.
7. Save/load policies and associate compatible models with each track.
8. Add a reinforcement-learning trainer using backpropagation and compare both autonomous approaches under common evaluation rules.

Multi-car display initially represents independent trials, without physical interactions between agents. Actual racing, overtaking, and avoidance of other cars will need additional observations and evaluation rules.

## Observed training results

These are results from the development examples, not autonomous lap-performance measurements or exhaustive gradient checks. Random initialization can produce different convergence curves.

| Exercise | Observed result |
| --- | --- |
| One Linear neuron learning `y = 2x + 1` | Loss about `1.52e-13`; prediction at `x = 0.5`: `2` |
| One Tanh neuron learning `y = tanh(x)` | Loss about `3.80e-12`; prediction at `x = 0.5`: `0.462115`, expected about `0.462117` |
| Network `1 -> 4 Tanh -> 1 Linear`, learning `y = 2x + 1` | Loss fell from `2.63022` to `0.000467665` after 3000 epochs; prediction at `x = 0.25`: `1.51397`, expected `1.5` |

The network regression example trains on 21 values from -1 to +1 in increments of 0.1; 0.25 is an additional interpolation check. These examples establish useful learning behaviour but do not prove correctness on every architecture or generalization beyond the tested domain.

Development checks also caught contradictory sample targets and an uninitialized layer-construction loop counter. Initialize loop counters (`std::size_t i = 0`) and remember that `reserve()` allocates capacity without creating elements.
