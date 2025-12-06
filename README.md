# Flappy Bird Clone 🐤🎮

**Flappy Bird Clone** is a small, arcade‑style game built in C with the raylib library. The game recreates the classic Flappy Bird feel with pixel‑art graphics, smooth tap‑to‑flap controls, capped pipes, and a retro arcade font, making it a great learning project for 2D game development with C and raylib. [web:141]

![Flappy Bird Banner](./githubAssets/banner.jpg)

---

## 🌟 Features

- **Classic Flappy Gameplay** – Tap to keep the bird in the air and fly through pipe gaps. [image:1]  
- **Smooth Start & Restart Flow** – Game waits for your first tap, then instantly restarts after each game over. [web:141]  
- **Pixel‑Perfect Pipes** – Custom 80×217 pipe texture with clean caps at the openings. [image:1]  
- **Retro UI** – Score and messages rendered with a pixel/arcade font for an old‑school vibe. [web:145]  
- **Simple State System** – `GAME_WAITING`, `GAME_RUNNING`, and `GAME_OVER` states keep logic clean. [web:141]  
- **Configurable Difficulty** – Pipe speed, spawn time, and gap size are all tunable constants. [web:141]

---

## 🚀 Technologies Used

- **C (ISO C99)** – Core game logic, physics, and state management. [web:141]  
- **raylib** – Lightweight C library for 2D rendering, input, textures, and audio. [web:141]  
- **Pixel Art Assets** – Bird, background, and pipe sprites drawn as simple pixel art. [image:1]  
- **Wave Audio** – `flap.wav`, `score.wav`, and `hit.wav` for feedback on game events. [web:141]

---

## 📁 Project Structure

```
FlappyBird/  
├── assets/  
│ ├── bg.png # Background (800x600)  
│ ├── bird.png # Bird sprite (27x20)  
│ ├── pipe.png # Pipe sprite (80x217, cap at top)  
│ ├── font.ttf # Pixel / arcade font  
│ └── sounds/  
│ ├── flap.wav  
│ ├── score.wav  
│ └── hit.wav  
└── src/  
├── main.c (Window + game loop)  
├── game.h (Game structs, constants, and API)  
├── game.c (Game logic, pipes, states, drawing)  
├── player.h (Bird API)  
└── player.c (Bird physics and rendering)  
```

---

## 🛠️ Installation & Setup

### Prerequisites

- GCC or another C compiler that supports C99. [web:141]  
- [raylib](https://www.raylib.com) installed (headers and libraries available on your system). [web:141]

### Steps

1. **Clone the repository**
```
git clone https://github.com/YourUsername/FlappyBird.git
cd FlappyBird
```


2. **Build the game (Windows example)**

```
gcc src/main.c src/game.c src/player.c ^
-I C:\raylib\include ^
-L C:\raylib\lib ^
-lraylib -lopengl32 -lgdi32 -lwinmm ^
-o FlappyBird.exe
```

On Linux/macOS, link against `raylib` and the appropriate system libraries as described in the raylib build instructions. [web:141]

3. **Run the game**

./FlappyBird.exe # Windows
./FlappyBird # Linux/macOS (name depends on your build)

---

## 🎮 How to Play

- **First Screen** – You’ll see “Press SPACE or Left Click” centered on the screen.  
- **Start** – Press **SPACE** or **Left Mouse Button** to begin; the bird starts falling immediately.  
- **Flap** – Press **SPACE** / **Left Click** to flap and stay in the air.  
- **Score** – Fly through the gaps between pipes to increase your score.  
- **Game Over** – Colliding with a pipe or the ground ends the run; “GAME OVER” and a restart hint appear.  
- **Restart** – Press **SPACE** / **Left Click** on the game‑over screen to instantly start a new run. [web:141]

---

## 🎨 Customization

### Tuning Physics & Difficulty

In `src/game.h`, you can adjust:

```
#define GRAVITY 420.0f
#define FLAP_STRENGTH -260.0f

#define PIPE_SPEED 160.0f
#define PIPE_SPAWN_TIME 2.0f

#define MIN_GAP_SIZE 135
#define MAX_GAP_SIZE 185
```

- Increase `PIPE_SPEED` or decrease `MIN_GAP_SIZE` for a harder game.  
- Tweak `GRAVITY` and `FLAP_STRENGTH` to change the feel of the bird. [web:141]

### Swapping Art & Fonts

- Replace `bg.png`, `bird.png`, or `pipe.png` with your own art (keep sizes similar for best results).  
- Drop in any pixel/arcade TTF as `assets/font.ttf` to change the UI style (e.g., Press Start 2P). [web:145]

---

## 🔮 Possible Future Enhancements

- Separate `pipe_up` / `pipe_down` textures for more detailed graphics.  
- Animated bird wings using sprite‑sheet frames.  
- Parallax backgrounds and day/night cycles.  
- High‑score saving to a file.  
- Mobile controls (touch/tap) or controller support. [web:141]

---

## 👨‍💻 Author

**Ronel Abraham Mathew**  
- GitHub: [RM1338](https://github.com/RM1338)  
- LinkedIn: [Ronel Abraham Mathew](https://linkedin.com/in/ronelm)

---

## ⭐ Show Your Support

If this project helped you learn raylib or C game dev, consider giving the repo a ⭐ on GitHub and experimenting with your own art, physics, and features!