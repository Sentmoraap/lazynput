#include <iostream>
#include "Lazynput/LazynputDb.hpp"
#include <math.h>
#include <SFML/Graphics.hpp>
#include <functional>

#ifdef LAZYNPUT_USE_SDL_WRAPPER
#include <SDL2/SDL.h>
#include "Lazynput/Wrappers/SdlWrapper.hpp"
#undef main
#endif

#ifdef LAZYNPUT_USE_GLFW_WRAPPER
#define GLFW_INLCUDE_NONE
#include <GLFW/glfw3.h>
#include "Lazynput/Wrappers/GlfwWrapper.hpp"
#endif

#ifdef LAZYNPUT_USE_SFML_WRAPPER
#include "Lazynput/Wrappers/SfmlWrapper.hpp"
#endif

using namespace Lazynput::Literals;

// Lazynput is not an input manager so this program needs a way to know when a button is just pressed.
struct NewpressAction
{
    bool canPress = false;
    bool newPressed;
    void update(bool pressed)
    {
        newPressed = false;
        if(pressed)
        {
            if(canPress) newPressed = true;
            canPress = false;
        }
        else canPress = true;
    }
};

// Global constants
static constexpr int GAME_SCALE = 32;
static constexpr int GAME_SIZE_X = 20;
static constexpr int GAME_SIZE_Y = 15;
static constexpr int FRAME_RATE = 60;
static constexpr float BORDER_WIDTH = 1.f;
static constexpr float RIGHT_WALL_X = GAME_SIZE_X - BORDER_WIDTH;

struct Bullet
{
    static constexpr float SPEED = 10.f;
    static constexpr float RADIUS = 0.25;

    float x, y;
    int8_t sx; // 0 = inactive

    void update()
    {
        if(sx)
        {
            x += sx * SPEED / FRAME_RATE;
            if(x <= BORDER_WIDTH || x >= RIGHT_WALL_X) sx = 0;
        }
    }

    void draw(sf::RenderWindow &window)
    {
        if(!sx) return;
        sf::CircleShape circle(RADIUS * GAME_SCALE);
        circle.setFillColor(sf::Color(192,64,64));
        circle.setPosition((x - RADIUS) * GAME_SCALE, (y - RADIUS) * GAME_SCALE);
        window.draw(circle);
    }
};

struct InputMapping
{
    char name[32];
    Lazynput::StrHash hash;
};

enum GameInput
{
    JUMP,
    SHOOT_LEFT,
    SHOOT_RIGHT,
    PLACE_BLOCK,
    DASH_LEFT,
    DASH_RIGHT,
    SHIFT_BLOCK_LEFT,
    SHIFT_BLOCK_RIGHT,
    PAUSE,
    QUIT,
    MAX = QUIT
};

// Map inputs to both a interface input. Unsupported devices will also have interface input mappings for the
// programmer's convinience, but those mappings can be wrong.
// An input config menu is needed but it's outside the scope of this example.
static const InputMapping inputMappings[10] =
{
    {"Jump", "basic_gamepad.a"_hash},
    {"Shoot left", "basic_gamepad.x"_hash},
    {"Shoot right", "basic_gamepad.b"_hash},
    {"Create/destroy block", "basic_gamepad.y"_hash},
    {"Dash left", "basic_gamepad.l1"_hash},
    {"Dash right", "basic_gamepad.r1"_hash},
    {"Shift blocs left", "basic_gamepad.l2"_hash},
    {"Shift blocs right", "basic_gamepad.r2"_hash},
    {"Pause", "basic_gamepad.start"_hash},
    {"Quit", "basic_gamepad.select"_hash}
};

int main(int argc, char **argv)
{
    // Constants
    static constexpr float FLOOR_Y = GAME_SIZE_Y - BORDER_WIDTH;
    static constexpr float CHARACTER_SIZE_X = 0.75f;
    static constexpr float CHARACTER_SIZE_Y = 1.5f;
    static constexpr float CAMERA_RADIUS = FLOOR_Y;
    static constexpr float TEXT_SIZE = 0.5;
    static constexpr float TEXT_OUTLINE_SIZE = 0.05;
    static constexpr float CHARACTER_SPEED = 4.f; // Units per second
    static constexpr float JUMP_HEIGHT = 2.f;
    static constexpr float JUMP_RISE_GRAVITY = 5; // Units per second²
    static constexpr float JUMP_FALL_GRAVITY = 10; // Units per second²
    static constexpr float INPUT_ALIGN_X = 5.f;
    static constexpr float INPUT_TEXT_X = 5.5f;
    static constexpr float DASH_DURATION = 0.2f;
    static constexpr float DASH_SPEED = CHARACTER_SPEED * 2;
    static constexpr int NB_BULLETS = 4;
    static constexpr float SHOOT_HEIGHT = CHARACTER_SIZE_Y * 0.5;
    static constexpr float SHOOT_COOLDOWN = 0.1;
    static constexpr float BLOCK_SIZE = 1.f;
    static constexpr float BLOCKS_OFFSET_SPEED = 3.f;
    static constexpr uint8_t NUM_BLOCKS_X = (GAME_SIZE_X - BORDER_WIDTH * 2) / BLOCK_SIZE;
    static constexpr uint8_t NUM_BLOCKS_Y = (GAME_SIZE_Y - BORDER_WIDTH * 2) / BLOCK_SIZE;
    static constexpr uint8_t BLOCK_HP = 2;
    static constexpr int NUM_FONTS = 3;

    // SFML primitives
    sf::Color borderColor(64, 64, 64);
    sf::RectangleShape fieldRect(sf::Vector2f((GAME_SIZE_X - 2 * BORDER_WIDTH) * GAME_SCALE,
            (GAME_SIZE_Y - 2 * BORDER_WIDTH) * GAME_SCALE));
    fieldRect.setPosition(BORDER_WIDTH * GAME_SCALE, BORDER_WIDTH * GAME_SCALE);
    fieldRect.setFillColor(sf::Color(64,64,192));
    sf::RectangleShape characterRect(sf::Vector2f(CHARACTER_SIZE_X * GAME_SCALE, CHARACTER_SIZE_Y * GAME_SCALE));
    characterRect.setFillColor(sf::Color(64,192,64));
    sf::RectangleShape blockRect(sf::Vector2f(BLOCK_SIZE * GAME_SCALE, BLOCK_SIZE * GAME_SCALE));
    sf::Color blockColors[BLOCK_HP] = { sf::Color(192, 192, 192), sf::Color(128, 128, 128)};

    // Text rendering
    // Load several fonts to support a lot of Unicode characters
    sf::Font fonts[NUM_FONTS];
    fonts[0].loadFromFile("assets/sansation_regular.ttf");
    fonts[1].loadFromFile("assets/NotoSansSymbols-Regular.ttf");
    fonts[2].loadFromFile("assets/NotoSansSymbols2-Regular.ttf");
    sf::Text text;
    text.setCharacterSize(GAME_SCALE * TEXT_SIZE);
    text.setOutlineThickness(TEXT_OUTLINE_SIZE * GAME_SCALE);
    sf::Color defaultTextColor(255,255,255);
    text.setOutlineColor(sf::Color(0,0,0));

    // Game state
    float characterX = BORDER_WIDTH + 1; // Left of character
    float characterY = FLOOR_Y; // Bottom of character
    float characterSpeedY = 0;
    bool useRiseGravity = false;
    float dashTimer = 0;
    float shootLeftTimer = 0, shootRightTimer = 0;
    float blocksOffset = 0;
    int8_t dashDirec = 0;
    int8_t moveDirec = 1;
    NewpressAction pauseAction, dashLeft, dashRight, shootLeft, shootRight, quitAction, placeBlockAction;
    bool paused = false;
    Bullet bullets[NB_BULLETS];
    uint8_t blocks[NUM_BLOCKS_X][NUM_BLOCKS_Y] = {};

    // Init window and main loop
    sf::RenderWindow window(sf::VideoMode(GAME_SCALE * GAME_SIZE_X, GAME_SCALE * GAME_SIZE_Y), L"Lazẏnput example");
    sf::Clock clock;
    float prevTime = clock.getElapsedTime().asSeconds() * FRAME_RATE;

    // Lazynput
    Lazynput::LazynputDb lazynputDb;
    lazynputDb.parseFromDefault(&std::cerr);

    #ifdef LAZYNPUT_USE_SDL_WRAPPER
        SDL_Init(SDL_INIT_GAMECONTROLLER);
        SDL_GameControllerAddMappingsFromFile("./gamecontrollerdb.txt");
        Lazynput::SdlWrapper sdlWrapper(lazynputDb);
        Lazynput::LibWrapper &libWrapper = sdlWrapper;
    #endif

    #ifdef LAZYNPUT_USE_GLFW_WRAPPER
        glfwInitHint(GLFW_JOYSTICK_HAT_BUTTONS, GLFW_FALSE);
        glfwInit();
        Lazynput::GlfwWrapper glfwWrapper(lazynputDb);
        Lazynput::LibWrapper &libWrapper = glfwWrapper;
    #endif

    #ifdef LAZYNPUT_USE_SFML_WRAPPER
        Lazynput::SfmlWrapper sfmlWrapper(lazynputDb);
        Lazynput::LibWrapper &libWrapper = sfmlWrapper; // To be sure this program uses only the parent class interface.
    #endif

    // Main loop
    while(window.isOpen())
    {
        // Events and inputs
        sf::Event event;
        while(window.pollEvent(event))
        {
            if(event.type == sf::Event::Closed) window.close();
        }
        libWrapper.update();
        auto getGameInput = [&libWrapper](GameInput gi)
        {
            return libWrapper.getInputValue(0, inputMappings[gi].hash) > 0;
        };
        quitAction.update(getGameInput(GameInput::QUIT));
        if(quitAction.newPressed) window.close();

        // Update
        float time = clock.getElapsedTime().asSeconds() * FRAME_RATE;
        if(time < prevTime + 1.f) sf::sleep(sf::seconds((prevTime + 1.f - time) / FRAME_RATE));
        pauseAction.update(getGameInput(GameInput::PAUSE));
        if(pauseAction.newPressed) paused = !paused;
        if(paused) prevTime = time;

        while(prevTime + 1.f <= time)
        {
            // Read inputs
            float dx = libWrapper.getInputValue(0, "basic_gamepad.dpx"_hash)
                    + libWrapper.getInputValue(0, "basic_gamepad.lsx"_hash);
            if(dx < -1) dx = -1; else if(dx > 1) dx = 1;
            dx *= CHARACTER_SPEED;
            if(dx > 0) moveDirec = 1; else if(dx < 0) moveDirec = -1;
            bool jumpInput = getGameInput(GameInput::JUMP);
            dashLeft.update(getGameInput(GameInput::DASH_LEFT));
            dashRight.update(getGameInput(GameInput::DASH_RIGHT));
            shootLeft.update(getGameInput(GameInput::SHOOT_LEFT));
            shootRight.update(getGameInput(GameInput::SHOOT_RIGHT));

            // Move & dash
            if(dashTimer == 0)
            {
                characterX += dx / FRAME_RATE;
                if(characterY >= FLOOR_Y)
                {
                    if(dashLeft.newPressed && !dashRight.newPressed)
                    {
                        dashTimer = DASH_DURATION;
                        dashDirec = -1;
                    }
                    else if(!dashLeft.newPressed && dashRight.newPressed)
                    {
                        dashTimer = DASH_DURATION;
                        dashDirec = 1;
                    }
                }
            }
            else
            {
                characterX += DASH_SPEED * dashDirec / FRAME_RATE;
                dashTimer -= 1.f / FRAME_RATE;
                if(dashTimer < 0) dashTimer = 0;
            }
            if(characterX < BORDER_WIDTH) characterX = BORDER_WIDTH;
            else if(characterX + CHARACTER_SIZE_X > RIGHT_WALL_X)
                    characterX = RIGHT_WALL_X - CHARACTER_SIZE_X;

            // Jump
            bool inAir = false;
            if(characterY >= FLOOR_Y && characterSpeedY >= 0)
            {
                characterY = FLOOR_Y;
                characterSpeedY = 0;
                if(jumpInput)
                {
                    characterSpeedY = - sqrt(2 * JUMP_HEIGHT * JUMP_RISE_GRAVITY);
                    useRiseGravity = true;
                }
            }
            else
            {
                inAir = true;
                if(!jumpInput) useRiseGravity = false;
                if(characterSpeedY < 0 && useRiseGravity) characterSpeedY += JUMP_RISE_GRAVITY / FRAME_RATE;
                else characterSpeedY += JUMP_FALL_GRAVITY / FRAME_RATE;
                characterY += characterSpeedY / FRAME_RATE;
            }

            // Shoot
            auto updateShoot = [&bullets, characterX, characterY](float timer, NewpressAction &action, int8_t direc)
            {
                if(timer <= 0 && action.newPressed)
                {
                    for(uint8_t i = 0; i < NB_BULLETS; i++) if(!bullets[i].sx)
                    {
                        timer = SHOOT_COOLDOWN;
                        bullets[i].sx = direc;
                        bullets[i].x = characterX + CHARACTER_SIZE_X / 2.f;
                        bullets[i].y = characterY - SHOOT_HEIGHT;
                        break;
                    }
                }
                timer -= 1.f / FRAME_RATE;
            };
            updateShoot(shootLeftTimer, shootLeft, -1);
            updateShoot(shootRightTimer, shootRight, 1);
            for(uint8_t i = 0; i < NB_BULLETS; i++) bullets[i].update();

            // No character/block collisions for now

            // Create/destroy blocks
            placeBlockAction.update(getGameInput(GameInput::PLACE_BLOCK));
            if(placeBlockAction.newPressed)
            {
                int8_t blockX = moveDirec > 0
                        ? ceil((characterX - blocksOffset - BORDER_WIDTH + CHARACTER_SIZE_X) / BLOCK_SIZE)
                        : floor((characterX - blocksOffset - BORDER_WIDTH - BLOCK_SIZE) / BLOCK_SIZE);
                if(blockX >= 0 && blockX < NUM_BLOCKS_X)
                {
                    int8_t blockY = floor((characterY - BORDER_WIDTH - CHARACTER_SIZE_Y * 0.5) / BLOCK_SIZE);
                    if(libWrapper.getInputValue(0, "basic_gamepad.dpy"_hash)
                            + libWrapper.getInputValue(0, "basic_gamepad.lsy"_hash) > 0.5f && !inAir)
                            blockY++;
                    if(blockY >=0 && blockY < NUM_BLOCKS_Y)
                    {
                        uint8_t &block = blocks[blockX][blockY];
                        block = block ? 0 : BLOCK_HP;
                    }
                }
            }
            blocksOffset += BLOCKS_OFFSET_SPEED * (libWrapper.getInputValue(0, "basic_gamepad.r2"_hash)
                    - (libWrapper.getInputValue(0, "basic_gamepad.l2"_hash))) / FRAME_RATE;
            if(blocksOffset < 0)
            {
                for(uint8_t y = 0; y < NUM_BLOCKS_Y; y++) blocks[0][y] = 0;
                if(blocksOffset <= -BLOCK_SIZE)
                {
                    blocksOffset += BLOCK_SIZE;
                    for(uint8_t x = 0; x < NUM_BLOCKS_X - 1; x++) for(uint8_t y = 0; y < NUM_BLOCKS_Y; y++)
                            blocks[x][y] = blocks[x + 1][y];
                    for(uint8_t y = 0; y < NUM_BLOCKS_Y; y++) blocks[NUM_BLOCKS_X - 1][y] = 0;

                }
            }
            else if(blocksOffset > 0)
            {
                for(uint8_t y = 0; y < NUM_BLOCKS_Y; y++) blocks[NUM_BLOCKS_X - 1][y] = 0;
                if(blocksOffset >= BLOCK_SIZE)
                {
                    blocksOffset -= BLOCK_SIZE;
                    for(uint8_t x = NUM_BLOCKS_X - 1; x > 0; x--) for(uint8_t y = 0; y < NUM_BLOCKS_Y; y++)
                            blocks[x][y] = blocks[x - 1][y];
                    for(uint8_t y = 0; y < NUM_BLOCKS_Y; y++) blocks[0][y] = 0;
                }
            }


            prevTime++;
        }

        // Draw
        float camX = libWrapper.getInputValue(0, "basic_gamepad.rsx"_hash);
        float camY = libWrapper.getInputValue(0, "basic_gamepad.rsy"_hash);
        float camSqrMagnitude = camX * camX + camY * camY;
        if(camSqrMagnitude > 1.f)
        {
            float mult = 1.f / sqrt(camSqrMagnitude);
            camX *= mult;
            camY *= mult;
        }
        sf::View view = window.getView();
        view.setCenter(view.getCenter() + sf::Vector2f(camX, camY) * CAMERA_RADIUS);
        window.setView(view);
        window.clear(borderColor);
        window.draw(fieldRect);

        // Blocks
        for(uint8_t x = 0; x < NUM_BLOCKS_X; x++) for(uint8_t y = 0; y < NUM_BLOCKS_Y; y++) if(blocks[x][y])
        {
            blockRect.setFillColor(blockColors[blocks[x][y] - 1]);
            blockRect.setPosition((BORDER_WIDTH + blocksOffset + x * BLOCK_SIZE) * GAME_SCALE,
                    (BORDER_WIDTH + y * BLOCK_SIZE) * GAME_SCALE);
            window.draw(blockRect);
        }

        // Character
        characterRect.setPosition(characterX * GAME_SCALE, GAME_SCALE * (characterY - CHARACTER_SIZE_Y));
        window.draw(characterRect);

        // Bullets
        for(uint8_t i = 0; i < NB_BULLETS; i++) bullets[i].draw(window);


        window.setView(window.getDefaultView());

        // Draw texts
        text.setFillColor(defaultTextColor);
        Lazynput::LibWrapper::DeviceStatus deviceStatus = libWrapper.getDeviceStatus(0);
        if(deviceStatus >= Lazynput::LibWrapper::DeviceStatus::UNSUPPORTED)
        {
            const Lazynput::Device &device = libWrapper.getDevice(0);
            text.setPosition(0, 0);
            sf::String deviceName(device.getName());
            if(deviceStatus == Lazynput::LibWrapper::DeviceStatus::FALLBACK)
                deviceName += " [fallback]";
            else if(deviceStatus == Lazynput::LibWrapper::DeviceStatus::UNSUPPORTED)
                deviceName += " [unsupported]";
            text.setString(deviceName);
            window.draw(text);
            float lineHeight = TEXT_SIZE * GAME_SCALE;
            bool hasDpad = device.hasInput("basic_gamepad.dpx"_hash);
            bool hasJoystick = device.hasInput("basic_gamepad.lsx"_hash);
            text.setString("Move");
            sf::FloatRect bounds = text.getLocalBounds();
            text.setPosition(INPUT_ALIGN_X * GAME_SCALE - bounds.width, lineHeight);
            window.draw(text);

            // For 2D inputs, use it's name if it's the same for both axes. If it's not the same and it's a supported
            // device, use a default name.
            auto label2d = [&device, deviceStatus]
                    (const std::string &defaultName, Lazynput::StrHash xAxis, Lazynput::StrHash yAxis)
            {
                Lazynput::LabelInfos label = device.getLabel(xAxis);
                std::string xStr = label.utf8;
                if(!xStr.empty() && label.hasLabel)
                {
                    std::string yStr = device.getLabel(yAxis).utf8;
                    if(xStr == yStr) return label;
                }
                label.hasColor = false;
                if(deviceStatus > Lazynput::LibWrapper::DeviceStatus::UNSUPPORTED) label.utf8 = defaultName;
                else label.utf8 = (xStr + " " + device.getLabel(yAxis).utf8).c_str();
                return label;
            };

            // Display a label. If one font can display it's Unicode characters, that font is used. If not, use the
            // ASCII string instead.
            auto displayText = [&fonts, &defaultTextColor, &text, &window](const Lazynput::LabelInfos &label)
            {
                sf::String str = sf::String::fromUtf8(label.utf8.begin(), label.utf8.end());

                // Select the first font containing all the characters
                int font = 0;
                for(; font < NUM_FONTS; font++)
                {
                    const sf::Glyph *noGlyph = &(fonts[font].getGlyph(0x104242, GAME_SCALE * TEXT_SIZE, false, 0));
                    bool ok = true;
                    for(uint32_t c : str)
                    {
                        const sf::Glyph *glyph = &(fonts[font].getGlyph(c, GAME_SCALE * TEXT_SIZE, false, 0));
                        if(glyph == noGlyph)
                        {
                            ok = false;
                            break;
                        }
                    }
                    if(ok) break;
                }
                if(font < NUM_FONTS)
                {
                    text.setFont(fonts[font]);
                    text.setString(str);
                }
                else
                {
                    // If no font found, use the ASII string instead
                    text.setFont(fonts[0]);
                    text.setString(label.ascii);
                }
                if(label.hasColor) text.setFillColor(sf::Color(label.color.r, label.color.g, label.color.b));
                else text.setFillColor(defaultTextColor);
                window.draw(text);
            };

            float textX = INPUT_TEXT_X * GAME_SCALE;
            if(hasDpad)
            {
                Lazynput::LabelInfos li = label2d("d-pad", "basic_gamepad.dpx"_hash, "basic_gamepad.dpy"_hash);
                text.setPosition(textX, lineHeight);
                displayText(li);
                textX += text.getLocalBounds().width;
            }
            if(hasDpad && hasJoystick)
            {
                text.setString(" or ");
                text.setFillColor(defaultTextColor);
                text.setFont(fonts[0]);
                text.setPosition(textX, lineHeight);
                window.draw(text);
                textX += text.getLocalBounds().width;
            }
            if(hasJoystick)
            {
                Lazynput::LabelInfos li = label2d("left joystick", "basic_gamepad.lsx"_hash, "basic_gamepad.lsy"_hash);
                text.setPosition(textX, lineHeight);
                displayText(li);
            }
            if(!hasDpad && !hasJoystick)
            {
                text.setString("nothing");
                text.setPosition(textX, lineHeight);
                window.draw(text);
            }
            uint8_t line = 2;
            if(device.hasInput("basic_gamepad.rsx"_hash))
            {
                text.setString("Look around");
                text.setFillColor(defaultTextColor);
                text.setFont(fonts[0]);
                bounds = text.getLocalBounds();
                text.setPosition(INPUT_ALIGN_X * GAME_SCALE - bounds.width, lineHeight * line);
                window.draw(text);
                Lazynput::LabelInfos li = label2d("right joystick", "basic_gamepad.rsx"_hash, "basic_gamepad.rsy"_hash);
                text.setPosition(INPUT_TEXT_X * GAME_SCALE, lineHeight * 2);
                displayText(li);
                line++;
            }
            for(uint8_t i = 0; i <= GameInput::MAX; i++)
            {
                text.setPosition(INPUT_TEXT_X * GAME_SCALE, lineHeight * line);
                if(!device.hasInput(inputMappings[i].hash)) continue;
                displayText(device.getInputInfos(inputMappings[i].hash).label);
                text.setFillColor(defaultTextColor);
                text.setFont(fonts[0]);
                text.setString(inputMappings[i].name);
                bounds = text.getLocalBounds();
                text.setPosition(INPUT_ALIGN_X * GAME_SCALE - bounds.width, lineHeight * line);
                window.draw(text);
                line++;
            }
        }
        else
        {
            text.setString("No joystick detected");
            text.setPosition(0,0);
            window.draw(text);
        }
        text.setFillColor(defaultTextColor);
        text.setFont(fonts[0]);
        if(paused)
        {
            text.setString("[catchy beat]");
            sf::FloatRect bounds = text.getLocalBounds();
            text.setPosition(0.5 * (GAME_SIZE_X * GAME_SCALE - bounds.width),
                    0.5 * (GAME_SIZE_Y * GAME_SCALE - bounds.height));
            window.draw(text);
        }

        window.display();
    }
    return 0;
}
