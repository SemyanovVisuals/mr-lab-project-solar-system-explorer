# Solar System Explorer 🪐
*MR Lab Project · TU Wien*

Solar System Explorer is an interactive, stereoscopic 3D model of the Solar System built in Unreal Engine for the large curved LED wall of the TU Wien MR Lab. The planets sit at their real positions for the current date, computed from astronomical data. The planets start out hidden under a dark layer, and neon arrows from the Sun guide the viewer to them. Flying up to a hidden planet and answering its quiz question reveals it, until the whole Solar System is uncovered. Along the way, the viewer can scroll decades back and forth in time to watch the planets move.

> [!IMPORTANT]
> **This project is designed to run on the stereoscopic LED wall in the [TU Wien MR Lab](https://www.vr.tuwien.ac.at/mrlab/).**
> The wall uses passive polarization technology, so the viewer needs to wear **polarized 3D glasses** to see the stereoscopic (3D) effect. The view is rendered through nDisplay and follows the viewer's head, tracked by the lab's Qualisys motion capture system, so the perspective stays correct as they move in front of the wall.
> Running the project on a regular monitor works for development, but without the head-tracked, stereoscopic depth that the experience is built around.

### Features
- Real-Time Planet Positions: Sun, Mercury, Venus, Earth, Moon, Mars, Jupiter, Saturn, Uranus and Neptune are placed using [Astronomy Engine](https://github.com/cosinekitty/astronomy), with orbits in the ecliptic plane and each planet oriented by its real pole and rotation.
- Readable Scale: Distances and radii are compressed with tunable exponents, so the outer planets stay reachable and small bodies stay visible next to the Sun, while angles and phases remain exact.
- Time Travel: Scroll up to 50 years into the past or future. The scroll speed doubles while held (from one day per second up to ten years per second), and pressing both time buttons jumps back to today. A time slider shows the simulated date.
- Discover & Unlock Game Mode: Every planet starts locked and hidden under a dark layer. Neon arrows from the Sun point to the planets that are still locked, so the viewer can find them. Answering a planet's quiz question correctly unlocks it: its arrow fades out, the dark layer disappears to reveal the planet, and the camera flies around it in a cinematic fly-by. The goal is to reveal all the planets.
- Neon Guide Arrows: Drawn at a constant screen width by a custom HLSL shader, so they stay readable at any zoom level.
- Free Flight: Fly freely through the system with speed that ramps up while the stick is held, and an optional movement lock.
- Gaze-Based Selection: The planet in the viewer's line of sight through the screen is highlighted. Locking onto it moves the viewer to face the planet and switches to quiz mode, where input is limited to answering the quiz.
- Cinematic Camera: Smooth auto-zoom that brings the selected planet into view, a fly-by around the planet after a correct answer, and re-centering when the quiz is closed.
- Head-Tracked LED Wall Rendering: nDisplay configuration with the tracked viewpoint and screen geometry driving off-axis projection, zoom distances and the selection cone.

### Controls (Gamepad)
| Input | Action |
| --- | --- |
| Left stick | Fly forward/backward |
| Right stick | Look around |
| Right trigger | Lock onto the highlighted planet and start its quiz (press again to unlock) |
| Left trigger | Toggle movement lock |
| LB / RB (hold) | Scroll time backward / forward |
| LB + RB | Jump back to the current date |
| D-pad up/down · A · B | Quiz mode: select answer · confirm · exit |

### Getting Started
1. Open `MRLab_FinalProject.uproject` with **Unreal Engine 5.6** (Windows or Linux) and let it compile the C++ module.
2. Make sure the nDisplay, Switchboard, DisplayClusterLaunch and QTM Connect for LiveLink plugins are enabled.
3. Open the default map `Content/Maps/SolarSystemLab1`.
4. In the MR Lab, launch the nDisplay configuration (`Content/Blueprints/NDC_Basic`) on the LED wall cluster via Switchboard, connect the Qualisys tracking through LiveLink, and put on the polarized glasses.

### Tech Stack
Unreal Engine 5.6 · C++ · Blueprints · nDisplay · Switchboard · Qualisys Tracking (QTM LiveLink) · Stereoscopic 3D LED Wall · Astronomy Engine · Custom HLSL Shaders · Enhanced Input (Gamepad)

### Project Structure
- `Source/MRLab_FinalProject/` – C++ gameplay code: `SolarSystemManager` (planet positions and time), `PlanetBase`, `MyPlayerController` (movement, targeting, time scroll), `CinematicCameraComponent`, `PlanetArrowsComponent`, `TimeSliderWidget`.
- `Shaders/NeonArrows.hlsl` – shader for the neon arrows.
- `Content/Blueprints/` – game mode, pawn, controller, planet and manager Blueprints, plus the nDisplay config.
- `Content/UI/` – time slider and planet quiz widgets. Quiz questions live in `Content/Data/DT_Quiz`.
