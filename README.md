# F1_race

A top-down racing simulation written in modern C++20 with OpenGL. It supports keyboard driving and now displays a population of cars controlled by independently randomized neural networks implemented from scratch. Population learning is the next stage.

This is a learning project: build a working simulation, implement neural networks from scratch, and compare autonomous driving learned through neuroevolution and reinforcement learning. Supervised examples are used to understand and check gradient-based training, rather than as the main driving strategy.

## Current progress

- GLFW window and OpenGL rendering using GLAD.
- A car with acceleration, braking, steering, and visible wheels.
- A fixed simulation timestep of `1 / 120` second.
- A camera following a car without rotating with it; the current population view follows the first agent.
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

- A rendering-independent `Simulation` with stepping, observations, collisions, and reset.
- An `Agent` owning a simulation and a random `8 -> 16 -> 16 -> 3` Tanh network.
- Twenty independently updated agents displayed together, without car-to-car collisions.
- A staggered two-file starting grid with per-agent reset positions.
- Track centreline length and wrapped position coordinate, verified by Catch2 tests.
- A Catch2 v3 / CTest suite for loss, neurons, and track progress functions.

The agents currently perform inference with random weights: they do not yet learn from their driving trials. Per-agent accumulated progress, fitness selection, mutations, and generation management remain to be implemented. Reinforcement learning, model persistence, track/model association, and configurable training display modes are also planned. Circuit shapes will be developed separately.

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

Run the executable from the project root because shader paths such as `shaders/vertex.glsl` are relative to the working directory. Include the implementation files for `Track`, `Renderer`, `Simulation`, `Agent`, and the ML classes in the executable's CMake source list. Headers alone do not compile their corresponding `.cpp` files automatically.

## Controls

| Key | Action |
| --- | --- |
| W | Accelerate |
| S | Brake |
| A | Steer left |
| D | Steer right |
| Escape | Request window closure |

These driving keys describe the keyboard-controlled mode. In the current population loop, networks generate the driving commands; keyboard driving commands are not passed to agents. Escape still closes the window. Forward motion is currently supported; reverse gear is not implemented. The window-close request uses `glfwSetWindowShouldClose()`, while `glfwWindowShouldClose()` reads that flag.

## Project layout

```text
F1_race/
├── CMakeLists.txt
├── README.md
├── note.txt
├── include/
│   ├── Application.hpp
│   ├── Agent.hpp
│   ├── Car.hpp
│   ├── CarObservation.hpp
│   ├── CarSensors.hpp
│   ├── Exception.hpp
│   ├── Renderer.hpp
│   ├── Simulation.hpp
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
├── tests/
│   ├── test_loss.cpp
│   ├── test_neuron.cpp
│   └── test_track.cpp
├── shaders/
│   ├── vertex.glsl
│   └── fragment.glsl
└── src/
    ├── Application.cpp
    ├── Agent.cpp
    ├── Car.cpp
    ├── Renderer.cpp
    ├── Simulation.cpp
    ├── Track.cpp
    ├── glad.c
    ├── main.cpp
    └── ML/
        ├── Layer.cpp
        ├── Network.cpp
        ├── Neuron.cpp
        └── testML.cpp
```

`CarObservation.hpp` and `CarSensors.hpp` are the observation headers introduced in the latest development steps. `Exception.hpp` and `note.txt` are also present. `testML.cpp` contains the ML examples, including single-neuron and whole-network training. `Simulation` and `Agent` are now part of the project; the trainer classes are still planned.

## Architecture

### Simulation, agents, and rendering

| Component | Current responsibility |
| --- | --- |
| `Application` | Initialize GLFW/OpenGL, own the track and agent vector, process events, accumulate time, advance agents, and coordinate rendering |
| `Simulation` | Own one car and its assigned start position; observe, step physics, roll back collisions, and reset |
| `Agent` | Own a simulation and network, decode commands, and track trial activity and elapsed time |
| `Car` | Store position, heading, speed, and steering angle; update motion from `CarControls` |
| `Track` | Provide road geometry, start position, collision checks, wall raycasts, reference heading, centreline length, and position coordinate |
| `Renderer` | Manage OpenGL resources, configure the camera, and draw the track and cars |
| `Vec2` | Shared two-dimensional coordinates independent of OpenGL |

`Simulation` contains no GLFW or OpenGL operations. Each simulation holds a `const Track&`, so the track must outlive all simulations referring to it. `Application` declares its track before its agent vector. Agents own independent car and network state while sharing immutable track geometry.

`Simulation::step(controls, dt)` saves the car state, calls `Car::update()`, and checks the candidate position. On collision it restores the saved state, stops the car, and returns `true`; otherwise it returns `false`. `reset()` reconstructs the car at that simulation's assigned start position. The default constructor path uses `track.startPosition()`; a second constructor accepts and validates an explicit grid position using the car's collision radius.

The current frame loop:

1. Poll events and accumulate elapsed time, limiting catch-up after a pause.
2. For each fixed `1 / 120` second tick, call `step()` on every agent.
3. Each active agent observes, runs its network, decodes controls, and advances its simulation once.
4. Deactivate an agent after collision or 20 seconds of simulated trial time.
5. Set the camera once, draw the shared track once, then draw all agents and swap buffers.

```cpp
while (accumulator >= fixedDt)
{
    for (Agent& agent : _agents)
        agent.step(static_cast<float>(fixedDt));

    accumulator -= fixedDt;
}

_renderer.beginFrame(_agents.front().car().position(), width, height);
_renderer.drawTrack(_track);

for (const Agent& agent : _agents)
    _renderer.drawCar(agent.car());
```

The rendering excerpt assumes the initialized population is nonempty and a valid framebuffer size has been checked. `Agent::step()` ignores inactive agents. Inactive cars remain visible at their final positions; there is no automatic next generation yet. `Agent::reset()` resets the trial without changing its network.

### Starting grid

The current circuit is `120 x 80` metres with a `20` metre road. Twenty agents start in two staggered files on the upper straight, facing +X. Relative to the original start centre:

```cpp
const float x = centre.x + 28.5f - 3.0f * i;
const float y = centre.y + ((i % 2 == 0) ? 3.0f : -3.0f);
```

Successive cars are offset by 3 metres longitudinally and alternate sides; cars in the same file are separated by 6 metres. This grid fits the current track and each spawn is checked with `canDrive()`. It is not yet a general grid generator for arbitrary circuits or population sizes.

Cars do not collide with one another: these are independent driving trials displayed together. Because positions differ, future fitness must measure progress relative to each agent's own start. Rotating grid assignments or evaluating candidates from common starts will help compare networks fairly.

### Planned training orchestration

`Application` remains the orchestrator. Circuit selection, training-method selection, optional ray display, and saved-model gameplay will be added around the existing simulation.

| Planned component | Responsibility |
| --- | --- |
| `EvolutionTrainer` | Score a population, select parents, preserve elites, mutate offspring, and launch generations |
| `SupervisedTrainer` | Organize input/target examples using the existing supervised training methods |
| `ReinforcementTrainer` | Implement a reward-based algorithm with a suitable gradient objective |

The simulation is now independent of rendering, but a separate headless training driver and display controls are not implemented yet. Keep physics at `1 / 120` second when adding rendering every N ticks, pause/single-step controls, or no-rendering mode. Rendering frequency must not change the physical timestep. Headless execution need not wait for real time.

The current 20 agents are advanced sequentially during each tick; multi-agent simulation does not require multithreading.

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

The collision circle is conservative relative to the drawn car, so contact can happen before a visible part reaches a wall. In keyboard mode, accelerating into a wall leaves the car blocked. In the current agent mode, collision ends the trial and the car remains at its last valid position. Recovery behaviour is future work.

### Resource lifetime

OpenGL resource creation occurs after the context is made current and GLAD is loaded. Renderer cleanup occurs before destroying that context; `_rendererReady` guards shutdown after successful initialization, and the window pointer starts as `nullptr`. GPU buffers and shader programs require explicit OpenGL deletion; the current design uses `Renderer::shutdown()` to control that ordering.

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

This reference heading changes abruptly at rectangular corners. A later track representation can provide a smoother tangent. It is an orientation observation. The separate `progressAt()` coordinate is now implemented; accumulated agent progress and checkpoints are still pending.

Observations are computed before the corresponding physics update:

```cpp
const auto observation = _simulation.observe();
const auto inputs = observation.normalized(40.0f, 50.0f);
const auto outputs = _network.forward(inputs);
// Decode outputs into CarControls, then call _simulation.step().
```

For the 120 x 80 metre track with a 10 metre road, the initial centre position and zero heading give distances approximately `{5, 7.071, 40, 7.071, 5}`. At rest, normalized inputs are approximately `{0.125, 0.1768, 1, 0.1768, 0.125, 0, 0, 1}`. These five distance measurements have been checked.

The current driving network has eight inputs, two hidden layers of 16 neurons, and three output neurons. Every computational layer uses Tanh. Output decoding in `Agent::step()` is:

| Output | Command | Decoding |
| --- | --- | --- |
| 0 | Steering | `clamp(output, -1, 1)` |
| 1 | Throttle | `clamp(output, 0, 1)` |
| 2 | Brake | `clamp(output, 0, 1)` |

Negative throttle/brake outputs become zero; they are not remapped from `[-1, 1]` to `[0, 1]`. The architecture has `(8*16+16) + (16*16+16) + (16*3+3) = 467` weights and biases. Its size remains an experimental choice.

A random network can ask for zero throttle and positive braking. This was observed during integration and correctly leaves the car stationary. Net acceleration is `8 * throttle - 15 * brake - 1` in metres per second squared, so positive throttle alone does not guarantee movement. These trials currently end at the 20-second limit; an earlier no-progress timeout is planned.

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
| Status | Population inference works; scoring, selection, and mutations pending | Planned; no specific algorithm selected yet |

Supervised learning is already implemented for numerical exercises and could support an optional imitation baseline. It needs known targets. A lap score cannot simply be passed as the three desired driving commands to the existing `train()` method.

### Neuroevolution: population and generations

The current population contains 20 independent agents with the same network architecture and independently randomized weights. Each already owns car state and a trial timer; fitness scores and generation management are the next additions. The staggered grid currently gives different starting positions, which must be accounted for during evaluation.

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

Track-position coordinates are now implemented and tested, as described below. Ordered checkpoints and a validated finish-line crossing are still planned. Collision and a 20-second maximum duration already end agent trials; fitness penalties and early termination after prolonged lack of progress are not implemented yet.

A possible per-step reward for later reinforcement learning combines signed route progress, elapsed-time cost, collision penalty, and a validated lap-completion bonus. Exact coefficients and fitness ranking remain to be chosen. Rewarding survival alone can favour stopping; rewarding speed alone can favour crashing; rewarding only completed laps gives beginners little feedback.

### Track coordinate: implemented and tested

`Track::length()` returns the perimeter of the rectangular centreline:

```text
centreWidth  = width  - roadWidth
centreHeight = height - roadWidth
length       = 2 * (centreWidth + centreHeight)
```

`Track::progressAt(position)` projects a position onto the nearest centreline segment and returns a clockwise coordinate in `[0, length)`. The origin is the midpoint of the upper straight, independently of each agent's grid position.

For the current `120 x 80` metre track with a 20 metre road, the centreline is `100 x 60` metres and its length is 320 metres:

| Position | Coordinate |
| --- | --- |
| `(0, 30)` — original start centre | 0 m |
| `(10, 30)` | 10 m |
| `(50, 30)` — upper-right corner | 50 m |
| `(50, 0)` | 80 m |
| `(50, -30)` — lower-right corner | 110 m |
| `(0, -30)` | 160 m |
| `(-50, -30)` — lower-left corner | 210 m |
| `(-50, 0)` | 240 m |
| `(-50, 30)` — upper-left corner | 270 m |
| `(-10, 30)` | 310 m |

The projection parameter `t` lies in `[0, 1]`. The distance from the first centreline corner is `distanceBeforeSegment + t * segmentLength`; the origin is then shifted to the upper midpoint and negative coordinates are wrapped by adding the circuit length.

This coordinate is not distance already travelled. An agent spawning at coordinate 310 has not earned 310 metres. Per-agent tracking still needs an initial coordinate, signed increments across the wrap (`319 -> 0` means +1 metre), accumulated progress, and best progress. Reverse movement must subtract progress rather than earn distance. Coordinate projection by itself does not validate a lap or prevent every shortcut; ordered checkpoints remain planned. Nearest-segment ambiguity near rectangular corners is also a limitation of this initial representation.

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

## Automated tests

The project uses Catch2 v3 and CTest. With `BUILD_TESTING` enabled, CMake finds Catch2 through `find_package(Catch2 3 CONFIG REQUIRED)`, builds `F1_tests`, links `Catch2::Catch2WithMain`, and registers cases using `catch_discover_tests(F1_tests)`.

The current test target includes:

```cmake
add_executable(F1_tests
    tests/test_loss.cpp
    tests/test_neuron.cpp
    tests/test_track.cpp
    src/ML/Neuron.cpp
    src/Track.cpp
)

target_include_directories(F1_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_features(F1_tests PRIVATE cxx_std_20)
target_link_libraries(F1_tests PRIVATE Catch2::Catch2WithMain)
```

Use this inside the existing `if(BUILD_TESTING)` block, with `include(CTest)` before it and `include(Catch)` / `catch_discover_tests(F1_tests)` after target configuration. Catch2 must be discoverable by CMake. These tests do not create a window or need an OpenGL context; this does not imply that the application's top-level CMake dependency discovery is already optional for headless environments.

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --target F1_tests
ctest --test-dir build --output-on-failure
```

To show the detailed track comparisons in the current Windows/MSYS2 environment:

```bash
./build/F1_tests.exe "[track]" --reporter compact
```

| Test file | Current checks |
| --- | --- |
| `test_loss.cpp` | Zero loss for exact predictions, known MSE value, rejected size mismatch, and rejected empty inputs |
| `test_neuron.cpp` | Weighted sum and Linear output, negative/positive ReLU behaviour, rejected input count, and a known supervised parameter update |
| `test_track.cpp` | Centreline length for several dimensions, zero at the original start, clockwise coordinates on all four sides and corners, wrap around the origin, and lateral projection |

Track tests use `Catch::Approx(...).margin(1e-4f)` for position coordinates and `CAPTURE()` for failing sample positions. All five track test cases pass after correcting the projection-distance expression.

For new functionality, prefer deterministic Catch2 checks over temporary debug output in `Application`. Add matching tests for accumulated progress, no-progress timeouts, and evolutionary operations as those features are implemented. Current coverage does not yet include full-network gradient checks or agent lifecycle/population tests.

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

Completed in the current development steps:

- Neurons, dense layers, networks, activations, MSE, and random initialization.
- Single-neuron training, activation derivatives, and full-network backpropagation examples.
- Wall-distance sensors, eight normalized observations, and track-relative heading.
- Rendering-independent simulation with collision reporting and per-position reset.
- Agents with individual random networks and explicit steering/throttle/brake decoding.
- A displayed population of 20 agents on a staggered starting grid.
- Trial termination on collision or a 20-second simulated-time limit.
- Track length and clockwise position coordinate, with passing Catch2 tests.
- Catch2/CTest integration for loss, neuron, and track checks.

Next implementation stages:

1. Track signed accumulated progress from each agent's own start, best progress, and lack-of-progress timeouts; add deterministic tests.
2. Add ordered checkpoints, lap validation, fitness ranking, and fair evaluation across starting positions.
3. Implement independently owned parameter copying, mutation, elite preservation, and generation management.
4. Add training controls in `Application`: track selection, headless execution, rendering interval, pause/single-step mode, selected-agent tracking, and optional ray display.
5. Save/load policies and associate compatible models with each track.
6. Add a reinforcement-learning trainer using backpropagation and compare both autonomous approaches under common evaluation rules.

The current population performs independent inference trials; it is not yet evolving. Actual racing, overtaking, and avoidance of other cars will need additional observations and evaluation rules.

## Observed training results

These are results from the development examples, not autonomous lap-performance measurements or exhaustive gradient checks. Random initialization can produce different convergence curves.

| Exercise | Observed result |
| --- | --- |
| One Linear neuron learning `y = 2x + 1` | Loss about `1.52e-13`; prediction at `x = 0.5`: `2` |
| One Tanh neuron learning `y = tanh(x)` | Loss about `3.80e-12`; prediction at `x = 0.5`: `0.462115`, expected about `0.462117` |
| Network `1 -> 4 Tanh -> 1 Linear`, learning `y = 2x + 1` | Loss fell from `2.63022` to `0.000467665` after 3000 epochs; prediction at `x = 0.25`: `1.51397`, expected `1.5` |

The network regression example trains on 21 values from -1 to +1 in increments of 0.1; 0.25 is an additional interpolation check. These examples establish useful learning behaviour but do not prove correctness on every architecture or generalization beyond the tested domain.

Development checks also caught contradictory sample targets and an uninitialized layer-construction loop counter. Initialize loop counters (`std::size_t i = 0`) and remember that `reserve()` allocates capacity without creating elements.
