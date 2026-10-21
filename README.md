# F1_race

A top-down racing simulation written in C++20 with OpenGL and a neural network implemented from scratch. Twenty independent driving agents now learn through neuroevolution: evaluate driving trials, select a parent, preserve an elite, mutate descendants, and repeat.

The project combines vehicle simulation, numerical observations, supervised backpropagation exercises, evolutionary driving, timed lap validation, and persistent champion/track files. A future reinforcement-learning implementation will provide a second autonomous learning approach for comparison.

## Current progress

- GLFW/OpenGL rendering with GLAD, a following camera, and keyboard driving support.
- Rendering-independent `Simulation`, fixed physics at `1 / 120` second, and circular wall collisions.
- Configurable rectangular tracks; the latest development circuit is `800 x 500` metres with a `20` metre road and a `2520` metre centreline.
- Five wall-distance sensors, speed, and track-relative heading encoded as eight normalized inputs.
- Independent agents using `8 -> 16 -> 16 -> 3` Tanh networks.
- Signed per-agent progress, best progress, and termination after insufficient progress.
- Complete evolutionary generations with selection, weight/bias mutation, independent copies, and elite preservation.
- Ordered lap checkpoints, lap count, and simulated-time lap timing in `Simulation`.
- Selection prioritizing a completed lap and then the shortest lap time; progress ranks unsuccessful trials.
- An in-memory global champion used as the parent after the first successful lap.
- Versioned champion files bundling the rectangular circuit, network, observation/control encodings, and record time.
- Automatic file update on a new record; read/write round-trip tests pass.
- Catch2 v3 / CTest tests for numerical ML, progress, selection, mutations, laps, and persistence.

Agents have been observed completing circuits after six generations in one development run. This is an observed result, not a guaranteed convergence time. Reinforcement learning, an accelerated training loop, parallel evaluation, and an application-level track/champion loading workflow remain planned.

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

Run the executable from the project root because shader paths such as `shaders/vertex.glsl` are relative to the working directory. Include the implementation files for `Track`, `Renderer`, `Simulation`, `Agent`, `ChampionIO`, the trackers, and the ML classes in the executable's CMake source list. Headers alone do not compile their corresponding `.cpp` files automatically.

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

The main components and test files introduced so far are shown below. Utility headers/sources may be grouped under `ML/` according to the local project layout; preserve the actual include paths used by the build.

| Area | Files and responsibilities |
| --- | --- |
| Application | `Application.hpp/.cpp`, `main.cpp`: orchestration, window/input, population evaluation, champion selection, rendering, and saving |
| World | `Car.hpp/.cpp`, `Track.hpp/.cpp`, `Simulation.hpp/.cpp`, `Vec2.hpp` |
| Observations | `CarObservation.hpp`, `CarSensors.hpp` |
| Agent | `Agent.hpp/.cpp`: policy inference, controls, progress, trial termination, and network replacement/mutation |
| Trackers | `ProgressTracker.hpp/.cpp`, `LapCounter.hpp` and its `.cpp` when definitions are separated |
| Persistence | `ChampionIO.hpp/.cpp`: circuit/champion serialization and reconstruction |
| Neural network | `ML/Activation.hpp`, `Loss.hpp`, `Neuron.hpp/.cpp`, `Layer.hpp/.cpp`, `Network.hpp/.cpp`, `Selection.hpp` |
| Rendering | `Renderer.hpp/.cpp`, `shaders/vertex.glsl`, `shaders/fragment.glsl`, `src/glad.c`, GLAD/GLM/KHR headers |
| Tests | `test_loss.cpp`, `test_neuron.cpp`, `test_track.cpp`, `test_progress.cpp`, `test_selection.cpp`, `test_mutation.cpp`, `test_lap.cpp`, `test_champion_io.cpp` |
| Supporting files | `Exception.hpp`, `CMakeLists.txt`, `README.md`, `note.txt`, and the ML demonstration file `testML.cpp` |
| Runtime output | `circuits/rectangle.f1`: circuit and champion bundle, created/updated on a new lap record |

## Architecture

### Ownership and simulation

| Component | Current responsibility |
| --- | --- |
| `Application` | Own the track and agent vector; process input/time; advance agents; evaluate generations; preserve/save the champion; coordinate rendering |
| `Simulation` | Own one car and assigned start position; observe, step, check collisions, count/time laps, and reset |
| `Agent` | Own a simulation, network, progress tracker, trial timer, and active state; expose results to the application |
| `ProgressTracker` | Convert wrapped track coordinates into signed accumulated progress, retain the record, and monitor lack of improvement |
| `LapCounter` | Validate ordered coordinate checkpoints and maintain lap timing/count |
| `Track` | Geometry, starting reference, collision checks, wall raycasts, local heading, centreline length, and wrapped coordinate |
| `ChampionIO` | Write/read the track, driving network, record, and encoding metadata |
| `Renderer` | Manage OpenGL resources and draw the track and cars |

`Simulation` has no GLFW or OpenGL operations. Its `const Track&` refers to the shared circuit, which must outlive all agents/simulations using it. Each agent owns independent vehicle and network state. `Application` declares its track before its population.

`Simulation::step(controls, dt)` saves the car state, applies motion, and checks the candidate position. Collision restores the previous state and stops the car. Lap tracking uses the retained position, so a rejected displacement does not count as a checkpoint crossing. The method still returns whether a collision occurred.

`Simulation::reset()` reconstructs the car at its assigned start and clears lap state. `Agent::reset()` also clears accumulated progress, inactivity timing, and trial duration, then reactivates the agent. Resetting an agent does not replace its network.

### Current execution loop

1. Poll events and accumulate elapsed real time.
2. At each fixed physics tick, advance every active agent once.
3. An agent observes, runs `forward()`, decodes controls, steps its simulation, and updates progress/termination state.
4. Report a lap event immediately after an active agent's step, even if that step ends its trial.
5. Once every agent is inactive, rank the population, update/save a new global record if appropriate, reproduce, and reset all trials.
6. Configure the camera once per image, draw the shared track once, and draw all cars.

Inactive cars wait until all trials end; generations currently restart together. The application skips inactive agents when reporting lap events so that an event from a frozen simulation is not printed repeatedly.

The camera follows the agent with the greatest current `progress()`, including an inactive leader. Fitness uses different rules, described below. Camera changes affect visualization, not learning.

### Starting positions

The staggered-grid implementation provides two files with 3 metre successive longitudinal offsets, 6 metre spacing within a file, and lateral offsets of +/-3 metres. The revised grid is entirely behind the start line:

```cpp
const Vec2 startPosition{
    centre.x - distanceBeforeLine
        - static_cast<float>(i) * longitudinalOffset,
    centre.y + side * lateralOffset
};
```

`distanceBeforeLine` was introduced as 6 metres. This prevents agents placed just after the line from driving almost a whole circuit before their first timed lap begins. Every spawn is validated using `Track::canDrive()` and the collision radius.

A common training start was also proposed for fairer policy comparisons:

```cpp
const Vec2 startPosition{
    centre.x - distanceBeforeLine,
    centre.y
};
```

Use this same position when constructing all agents if identical initial conditions are desired; retain the staggered grid for a race presentation. Cars have no mutual collisions in the current independent-trial setup. Different start positions can influence speed at the timing line and trial opportunity, even though each lap is timed individually.

### Planned orchestration and performance work

Evolution currently runs in `Application`; a separate `EvolutionTrainer` is an architectural refactoring still to come. A future `ReinforcementTrainer` will provide reward-based gradient learning. The supervised methods already exist independently of either trainer class.

The current accumulator follows real time. Hiding cars does not by itself accelerate training: if elapsed real time feeds the accumulator, 120 simulated seconds still take about 120 real seconds.

An accelerated mode must run fixed ticks without waiting for real time, optionally render after batches, process window events periodically, and avoid a vertical-sync wait per simulation tick. Keep `dt = 1 / 120`; increase the number of ticks per real second, not their simulated duration. This mode is not implemented yet.

Parallel evaluation is also planned. Each agent can advance its own state while reading shared track geometry; selection/reproduction must wait for the evaluations. OpenGL remains on its context-owning thread. The current population is updated sequentially, and no multithreading speedup has been measured.

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

The driving plan has changed from imitation to autonomous trial and evaluation. The current autonomous approach is neuroevolution; the planned second approach is reinforcement learning using backpropagation. Both will share observations, control meanings, physics, and evaluation rules. Imitation remains a possible optional baseline, but recording player demonstrations is no longer the next implementation step.

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

This reference heading changes abruptly at rectangular corners. A later track representation can provide a smoother tangent. It is an orientation observation. The separate `progressAt()` coordinate feeds the implemented progress tracker and lap counter.

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

A random network can ask for zero throttle and positive braking. This was observed during integration and correctly leaves the car stationary. Net acceleration is `8 * throttle - 15 * brake - 1` in metres per second squared, so positive throttle alone does not guarantee movement. These trials now end after 3 simulated seconds without sufficient record improvement. The general trial-duration limit was increased from 20 to 120 simulated seconds for the larger circuit; a completed first lap also ends the trial.

## Neural-network implementation

### Class responsibilities

| Component | Responsibility |
| --- | --- |
| `Neuron` | Own weights, a bias, and an activation; compute one output and backpropagate gradients |
| `Layer` | Own several neurons; compute outputs and sum their input-gradient contributions |
| `Network` | Own layers; run inference and supervised training through the full network |
| `Activation.hpp` | Define Linear, ReLU, and Tanh and their derivatives |
| `Loss.hpp` | Compute mean squared error |

Validation failures use the project's custom exceptions, notably `neuralError` for ML and serialization checks. Tests must expect the same type as the implementation.

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

Backpropagation is a gradient-computation mechanism, not a complete reward-based learning algorithm. The implemented evolutionary approach and planned reinforcement approach use different ways to obtain parameters, while sharing inference and simulation.

| Aspect | Neuroevolution: first implementation | Reinforcement learning: later comparison |
| --- | --- | --- |
| Experience | Complete driving trials from a population of networks | Interactions containing observations, actions, rewards, and episode endings |
| Learning signal | Fitness score per trial | Rewards and an algorithm-specific learning objective |
| Parameter changes | Parent selection, copying, and random mutations | Gradients calculated by backpropagation with a suitable optimizer |
| Desired command labels | Not required | Not required |
| Existing `forward()` | Reused | Reused |
| Existing MSE `train()` alone | Not used | Insufficient to implement reward-based driving |
| Status | Implemented: generations, mutation, lap-aware selection, and persistent champion | Planned; no specific algorithm selected yet |

Supervised learning is already implemented for numerical exercises and could support an optional imitation baseline. It needs known targets. A lap score cannot simply be passed as the three desired driving commands to the existing `train()` method.

### Neuroevolution: population and generations

Twenty networks share the architecture `8 -> 16 -> 16 -> 3`. Initially their weights are random. At generation end, `selectBestIndex(std::span<const AgentScore>)` selects a result according to these rules:

1. A completed validated lap beats an incomplete trial.
2. Among completed laps, the shorter lap time wins.
3. If neither candidate completed a lap, higher `bestProgress()` wins.
4. Exact ties preserve the first candidate.

The older overload taking `std::span<const float>` remains for numerical-score selection and its tests. `AgentScore` contains `completedLap`, `lapTime`, and `progress`.

Before any successful lap, the best-progress network is the parent. After a successful lap, `Application` stores a global champion in `std::optional<Network>` and its record time. A successful candidate replaces it only with a strictly shorter lap time. `evolvePopulation()` copies that parent before touching the population.

The diversified reproduction schedule for 20 agents is:

| Group | Count | Mutation probability | Perturbation standard deviation |
| --- | --- | --- | --- |
| Unchanged elite | 1 | 0 | 0 |
| Small mutations | 6 | 0.05 | 0.03 |
| Medium mutations | 6 | 0.10 | 0.10 |
| Large mutations | 5 | 0.20 | 0.30 |
| Exploration slots | 2 | New random networks by default | Not applicable |

An optional variant replaces the final two random networks with strongly mutated champion copies once a champion exists. Both configurations were discussed; use the branch actually selected in `evolvePopulation()`. Mutated copies are not guaranteed to finish a lap even when their parent does.

`Network::mutate()` traverses layers by reference; `Layer::mutate()` traverses neurons by reference. Each neuron's weights and bias are independently selected with a Bernoulli probability and receive a zero-mean normal perturbation. The generator is passed by reference. Zero probability or zero strength leaves parameters unchanged; invalid parameters raise the project's custom exception.

$$
w_{\mathrm{child}}=w_{\mathrm{parent}}+\varepsilon
$$

Normal perturbations have no hard amplitude bound. `strength` is a standard deviation. Ordinary value copies of the nested vectors preserve independent parameter storage, so mutating a child does not modify its parent. `Agent::setNetwork()`, `network()`, `mutateNetwork()`, and `randomizeNetwork()` support reproduction without copying the simulation or its start assignment.

Generation transitions occur only after all agents become inactive. The elite is not mutated; descendants are evaluated afresh and do not inherit the parent's score. On an unchanged circuit with unchanged starting conditions, the elite can repeat an identical result; a plateau alone does not establish a mutation defect.

### Progress and termination

`Track::progressAt(position)` returns the clockwise coordinate of the nearest projection on the rectangular centreline, in `[0, Track::length())`. Its origin is the upper-straight midpoint. It is a position coordinate, not the distance a car has already driven.

```text
centreWidth  = width  - roadWidth
centreHeight = height - roadWidth
length       = 2 * (centreWidth + centreHeight)
```

For `Track{800, 500, 20}`, centreline dimensions are `780 x 480` metres and length is `2520` metres. The smaller `120 x 80`, 20 metre-road circuit remains a test fixture with length `320` metres.

`ProgressTracker` starts every agent at progress zero, regardless of its initial coordinate. It accumulates signed differences using `std::remainder(current - previous, length)`: on a 320 metre circuit, `319 -> 1` contributes +2 metres and `1 -> 319` contributes -2 metres. The displacement between samples must be less than half the circuit length.

`progress()` is signed accumulated advancement; `bestProgress()` retains its maximum, initially zero. Reversing and returning to a previous maximum earns no new record. The tracker also accumulates simulated time without sufficient record improvement. At least 0.5 metres of new best progress renews the 3-second allowance; multiple smaller advances can accumulate to that threshold.

A current agent trial ends on any of:

- Collision with a track wall.
- First validated completed lap.
- 3 simulated seconds without at least 0.5 metres of further record improvement.
- Maximum duration of 120 simulated seconds.

The earlier 20-second cap caused apparent stagnation on the enlarged circuit even when a car was still driving correctly. Hiding the cars did not remove that cap. Termination reasons must be considered before interpreting a plateau as failed learning.

Centreline projection is an approximation near corners and does not establish geometric checkpoint-gate intersection. Signed tracking discourages simple back-and-forth distance farming, but is not a proof against every shortcut or projection artifact.

### Lap validation and timing

`LapCounter` uses ordered coordinate checkpoints at `0`, `L/4`, `L/2`, and `3L/4`. It expects the next checkpoint, accepts forward crossings, and completes a lap at the next valid return to zero after the three intermediate checkpoints. Crossings are inferred from coordinate increments; each increment must be shorter than a quarter of the circuit. Timing interpolates the crossing within a physics step, assuming coordinate advancement is linear within that step.

On reset, a car exactly at coordinate zero starts timing immediately. Any other start must first cross zero to arm the timer. That initial approach is not a completed lap. Consequently, a car spawning just after zero could travel almost two circuit lengths before its first counted lap; moving the grid behind the line fixes that presentation without adding a false lap.

`Simulation` exposes:

| Value | Meaning |
| --- | --- |
| `lapCount()` | Number of validated completed laps in the current trial |
| `timePerTrack()` / `_timePerTrack` | Elapsed time within the current timed lap; zero before timing starts |
| `lastLapTime()` / `_lastLapTime` | Duration of the most recently completed lap |
| `trackFinished()` / `_trackFinished` | Event flag for the simulation step that completed a lap |

The event flag is refreshed on each simulation step. The application reads it immediately after advancing an active agent; inactive simulations are not advanced and their last event should not be repeatedly reported. `Agent` forwards lap count, last lap time, and the completion event to `Application`.

The first-lap termination policy makes candidates comparable without selecting by how many laps fit within a fixed window. Trials still finish as a generation together: the first finisher is not selected immediately because another agent may record a shorter individually timed lap.

A future reinforcement algorithm can reuse these signals, but will need an explicit reward definition and gradient objective. The evolutionary fitness above is already implemented; a reinforcement reward is not.

### Comparing the approaches

Use the same track versions, observation encodings, control meanings, collision rules, and evaluation scenarios. Where applicable, use the same policy-network architecture. Evaluate completion rate, successful lap times, progress, total environment steps, and training wall-clock time with rendering disabled. Count the steps of every agent, not just the number of generations. Repeat experiments with recorded seeds and report failed trials as well as successful laps.

## Champion and circuit persistence

`ChampionIO` now writes and reads one text-format bundle containing both the rectangular circuit and its champion. It has friend access to `Track`, `Network`, `Layer`, and `Neuron` so serialization does not require public mutable parameter getters.

```text
circuits/
    rectangle.f1
    circuit_02.f1
```

`rectangle.f1` is the implemented output example; the second file illustrates future independent circuit/champion bundles. The folder is created when needed and the current record file is updated after a shorter validated lap. Assign a distinct path to each circuit instead of overwriting another circuit's champion.

Version 1 uses these records:

| Record | Stored data |
| --- | --- |
| `F1_CHAMPION 1` | File format/version |
| `RECTANGLE` | Width, height, and road width |
| `OBSERVATIONS 1 40 50` | Observation encoding version, 40 metre range, 50 m/s speed normalization |
| `CONTROLS 1` | Version of the three-output control decoding |
| `LAP_TIME` | Champion's validated lap time |
| `LAYERS` / `NEURONS` | Network topology |
| `NEURON` | Activation name, bias, input count, and weights |
| `END` | End of the bundle |

Activations are stored as `Linear`, `ReLU`, or `Tanh`. Numeric output uses the classic locale and `std::numeric_limits<float>::max_digits10` to preserve floats on round trip. `write(std::ostream&, ...)` and `read(std::istream&)` support both file streams and in-memory test streams.

`read()` reconstructs a `SavedChampion` containing `Track`, `Network`, and `lapTime`. It checks the version and record labels, compatible observation/control metadata, positive finite lap time, dimensions and layer continuity, eight inputs and three final outputs, supported activations, and trailing content. Size limits bound layer/neuron counts and total parameters. Errors use custom exceptions such as `neuralError`; diagnostic token errors distinguish the expected and actual record.

The current encoding versions stand for the agreed observation order and control mapping; they are not a full vehicle-physics fingerprint. Geometry is bundled directly, so it reconstructs the associated rectangular circuit. Future circuit shapes will need a new geometry representation and compatible format evolution. A physics/rules change can invalidate a previous record even if the current loader accepts the file.

The application already saves new records, and save/read tests pass. A startup workflow for selecting a bundle and choosing replay or resumed training is still pending. When applying a loaded circuit, rebuild the agents and their trackers rather than reusing trackers initialized for another circuit length.

Saving the champion is not a complete training checkpoint: exact resumption would also require population parameters, generation state, random-generator states, and any additional trainer state. Current record writes replace the file directly; interruption-safe replacement is a possible future improvement.

## Automated tests

The project uses Catch2 v3 and CTest. With `BUILD_TESTING` enabled, CMake finds Catch2, builds `F1_tests`, links `Catch2::Catch2WithMain`, and registers cases with `catch_discover_tests(F1_tests)`. Tests use the project's custom exception types, including `neuralError`, rather than expecting standard-library exception types from application validation.

Include test files and the implementation sources they exercise in the test target. For example, persistence tests need `ChampionIO`, `Track`, `Neuron`, `Layer`, and `Network`; progress/lap tests also need tracker implementation sources when those definitions are in `.cpp` files. Preserve the actual local paths under `src/` or `src/ML/`.

```cmake
include(CTest)

if(BUILD_TESTING)
    find_package(Catch2 3 CONFIG REQUIRED)

    # Define F1_tests with the test and implementation sources.
    # Keep the source list or existing source discovery up to date.

    target_include_directories(F1_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_compile_features(F1_tests PRIVATE cxx_std_20)
    target_link_libraries(F1_tests PRIVATE Catch2::Catch2WithMain)

    include(Catch)
    catch_discover_tests(F1_tests)
endif()
```

This excerpt shows the registration settings, not a complete replacement for the current `add_executable()` source configuration. Numerical, selection, tracker, and persistence tests do not create an OpenGL context. Top-level application dependency discovery may still require GLFW/OpenGL packages.

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

For detailed failures in the current Windows/MSYS2 environment:

```bash
./build/F1_tests.exe "[progress]" --reporter compact
./build/F1_tests.exe "[selection]" --reporter compact
./build/F1_tests.exe "[lap]" --reporter compact
./build/F1_tests.exe "[champion]" --reporter compact
```

| Test file | Covered behaviour |
| --- | --- |
| `test_loss.cpp` | Exact predictions, a known MSE, size mismatch, and empty inputs |
| `test_neuron.cpp` | Weighted sum, activations, input-count rejection, a supervised update, mutation no-ops, seeded reproducibility, parent preservation, and invalid mutation arguments |
| `test_track.cpp` | Centreline lengths, origin, clockwise side/corner coordinates, wrap, and lateral projection |
| `test_progress.cpp` | Start-relative progress, signed wrap, reversing, multi-wrap accumulation, reset, inactivity timeout, cumulative small improvements, and no renewal for returning to an old record |
| `test_selection.cpp` | Numerical-score selection plus lap-success priority, shorter lap time, fallback progress, ties, and empty populations |
| `test_mutation.cpp` | Full-network mutation propagation and independent parent/copy behaviour |
| `test_lap.cpp` | Ordered checkpoints, no false lap from reversing at the line, partial initial approach, crossing-time interpolation, event behaviour, and reset |
| `test_champion_io.cpp` | Round-trip predictions/circuit/record, rejected format version, truncated input, and invalid record time |

The development suite has been reported passing after these additions. Tests use floating-point tolerances where appropriate; serialization checks exact prediction equality after round-tripping float parameters in the same runtime. This is functional coverage, not exhaustive gradient verification or a full automated integration test of training convergence and rendering.

GitHub Actions is configured for pushes, pull requests, and manual dispatch using Windows and MSYS2 UCRT64. Keep newly added sources included in the current test-target configuration so that CTest can discover their cases.

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

Completed:

- Dense network inference, configurable activations, MSE, random initialization, and supervised backpropagation examples.
- Rendering-independent simulation, sensors, eight observations, and driving-output decoding.
- Twenty agents with independent state and networks; following camera and start-position handling.
- Signed accumulated progress, best progress, and no-progress termination.
- Ordered coordinate checkpoints, timed laps, and first-lap trial termination.
- Evolutionary generations with elite preservation, diversified mutations, and optional fresh random candidates.
- Lap-aware selection, global champion retention, and new-record reporting.
- Champion/circuit bundle writing and reading, with automatic record-file updates.
- Catch2/CTest coverage of the numerical, selection, tracker, mutation, and persistence features.

Next:

1. Integrate startup loading: choose a saved circuit/champion, replay it, or resume training from its policy.
2. Add accelerated training independent of elapsed real time, with optional rendering after batches and event processing.
3. Profile inference, raycasts, allocations, and synchronization before optimizing or parallelizing agent evaluation.
4. Add configurable track paths, training settings, pause/single-step controls, and optional sensor-ray display.
5. Introduce additional circuit geometry and its serialization without mixing incompatible policies or records.
6. Separate the training orchestration into a trainer and add interruption-safe persistence or full population checkpoints when needed.
7. Implement reinforcement learning using backpropagation and compare it with neuroevolution under shared evaluation rules.

The current independent trials have no car-to-car interaction. Racing, overtaking, and avoidance will require additional observations and evaluation rules.

## Observed training results

The numerical results below are development examples, not exhaustive gradient checks. Random initialization can produce different convergence curves.

| Exercise | Observed result |
| --- | --- |
| One Linear neuron learning `y = 2x + 1` | Loss about `1.52e-13`; prediction at `x = 0.5`: `2` |
| One Tanh neuron learning `y = tanh(x)` | Loss about `3.80e-12`; prediction at `x = 0.5`: `0.462115`, expected about `0.462117` |
| Network `1 -> 4 Tanh -> 1 Linear`, learning `y = 2x + 1` | Loss fell from `2.63022` to `0.000467665` after 3000 epochs; prediction at `x = 0.25`: `1.51397`, expected `1.5` |

The network regression example trains on 21 values from -1 to +1 in increments of 0.1; 0.25 is an additional interpolation check. These examples establish useful learning behaviour but do not prove correctness on every architecture or generalization beyond the tested domain.

Development checks also caught contradictory sample targets and an uninitialized layer-construction loop counter. Initialize loop counters (`std::size_t i = 0`) and remember that `reserve()` allocates capacity without creating elements.

### Autonomous driving observations

One development run produced a car completing circuits after six generations. Increasing the trial cap from 20 to 120 simulated seconds removed an artificial early stop on the enlarged circuit. Before first-lap termination was introduced, logged trials included a validated lap of about `49.7255 s` on `Track{800, 500, 20}`. This is a recorded example, not a certified fastest lap or the latest saved record.

Selection now prefers completed laps and then shorter times, and successful champion serialization has been exercised in the application. The camera switch only changed visualization; it did not itself improve the networks. Training performance and convergence must be assessed across repeated seeds and consistent starts, rather than from one successful run.
