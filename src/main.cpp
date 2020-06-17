#include <iostream>
#include "Lazynput/LazynputDb.hpp"
#include <math.h>
#include <SFML/Graphics.hpp>
#include <functional>

#ifdef USE_SDL
#include <SDL2/SDL.h>
#include "Lazynput/Wrappers/SdlWrapper.hpp"
#else
#include "Lazynput/Wrappers/SfmlWrapper.hpp"
#endif

using namespace Lazynput::Litterals;

// Lazynput is not an input manager so this program needs a way to know when a button is just pressed.
struct NewpressAction
{
    bool canPress;
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
    char name[16];
    Lazynput::StrHash hash;
    uint8_t button;
};

enum GameInput
{
    JUMP,
    SHOOT,
    SLASH,
    DASH_LEFT,
    DASH_RIGHT,
    PAUSE,
    QUIT,
    MAX = QUIT
};

// Map inputs to both an interface input and a unmapped button number.
// A game that uses Lazynput should handle devices not present in the database.
// An input config menu is needed but it's outside the scope of this example.
static const InputMapping inputMappings[7] =
{
    {"Jump", "basic_gamepad.a"_hash, 0},
    {"Shoot", "basic_gamepad.x"_hash, 2},
    {"Slash", "basic_gamepad.b"_hash, 1},
    {"Dash left", "basic_gamepad.l1"_hash, 4},
    {"Dash right", "basic_gamepad.r1"_hash, 5},
    {"Pause", "basic_gamepad.start"_hash, 9},
    {"Quit", "basic_gamepad.select"_hash, 8}
};

int main(int argc, char **argv)
{
    // Constants
    static constexpr float FLOOR_Y = GAME_SIZE_Y - BORDER_WIDTH;
    static constexpr float CHARACTER_SIZE_X = 0.75f;
    static constexpr float CHARACTER_SIZE_Y = 1.5f;
    static constexpr float TEXT_SIZE = 0.5;
    static constexpr float TEXT_OUTLINE_SIZE = 0.05;
    static constexpr float CHARACTER_SPEED = 4.f; // Units per second
    static constexpr float JUMP_HEIGHT = 2.f;
    static constexpr float JUMP_RISE_GRAVITY = 5; // Units per second²
    static constexpr float JUMP_FALL_GRAVITY = 10; // Units per second²
    static constexpr float INPUT_TEXT_X = 3;
    static constexpr float DASH_DURATION = 0.2f;
    static constexpr float DASH_SPEED = CHARACTER_SPEED * 2;
    static constexpr int NB_BULLETS = 4;
    static constexpr float SHOOT_HEIGHT = CHARACTER_SIZE_Y * 0.5;
    static constexpr float SHOOT_COOLDOWN = 0.1;

    // SFML primitives
    sf::Color borderColor(64, 64, 64);
    sf::RectangleShape fieldRect(sf::Vector2f((GAME_SIZE_X - 2 * BORDER_WIDTH) * GAME_SCALE,
            (GAME_SIZE_Y - 2 * BORDER_WIDTH) * GAME_SCALE));
    fieldRect.setPosition(BORDER_WIDTH * GAME_SCALE, BORDER_WIDTH * GAME_SCALE);
    fieldRect.setFillColor(sf::Color(64,64,192));
    sf::RectangleShape characterRect(sf::Vector2f(CHARACTER_SIZE_X * GAME_SCALE, CHARACTER_SIZE_Y * GAME_SCALE));
    characterRect.setFillColor(sf::Color(64,192,64));
    sf::Font font;
    font.loadFromFile("assets/sansation_regular.ttf");
    sf::Text text;
    text.setFont(font);
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
    float shootTimer = 0;
    int8_t dashDirec = 0;
    int8_t moveDirec = 1;
    NewpressAction pauseAction, dashLeft, dashRight, shootAction;
    bool paused = false;
    Bullet bullets[NB_BULLETS];

    // Init window and main loop
    sf::RenderWindow window(sf::VideoMode(GAME_SCALE * GAME_SIZE_X, GAME_SCALE * GAME_SIZE_Y), L"Lazẏnput example");
    sf::Clock clock;
    float prevTime = clock.getElapsedTime().asSeconds() * FRAME_RATE;

    // Lazynput
    Lazynput::LazynputDb lazynputDb;
    lazynputDb.parseFromDefault(&std::cerr);

    #ifdef USE_SDL
        SDL_Init(SDL_INIT_GAMECONTROLLER);
        SDL_GameControllerAddMappingsFromFile("./gamecontrollerdb.txt");
        Lazynput::SdlWrapper sdlWrapper(lazynputDb);
        Lazynput::LibWrapper &libWrapper = sdlWrapper;
    #else
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
        bool isSupported = libWrapper.getDeviceStatus(0) != Lazynput::LibWrapper::DeviceStatus::UNSUPPORTED;
        auto getGameInput = [&libWrapper, isSupported](GameInput gi)
        {
            return isSupported ? libWrapper.getInputValue(0, inputMappings[gi].hash) > 0
                    : libWrapper.getBtnPressed(0, inputMappings[gi].button);
        };
        if(getGameInput(GameInput::QUIT)) window.close();

        // Update
        float time = clock.getElapsedTime().asSeconds() * FRAME_RATE;
        if(time < prevTime + 1.f) sf::sleep(sf::seconds((prevTime + 1.f - time) / FRAME_RATE));
        pauseAction.update(getGameInput(GameInput::PAUSE));
        if(pauseAction.newPressed) paused = !paused;
        if(paused) prevTime = time;

        while(prevTime + 1.f <= time)
        {
            // Read inputs
            float dx = isSupported ? (libWrapper.getInputValue(0, "basic_gamepad.dpx"_hash)
                    + libWrapper.getInputValue(0, "basic_gamepad.lsx"_hash))
                    : (libWrapper.getAbsValue(0, 0) + libWrapper.getHatValues(0, 0).first);
            if(dx < -1) dx = -1; else if(dx > 1) dx = 1;
            dx *= CHARACTER_SPEED;
            if(dx > 0) moveDirec = 1; else if(dx < 0) moveDirec = -1;
            bool jumpInput = getGameInput(GameInput::JUMP);
            dashLeft.update(getGameInput(GameInput::DASH_LEFT));
            dashRight.update(getGameInput(GameInput::DASH_RIGHT));
            shootAction.update(getGameInput(GameInput::SHOOT));

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
                if(!jumpInput) useRiseGravity = false;
                if(characterSpeedY < 0 && useRiseGravity) characterSpeedY += JUMP_RISE_GRAVITY / FRAME_RATE;
                else characterSpeedY += JUMP_FALL_GRAVITY / FRAME_RATE;
                characterY += characterSpeedY / FRAME_RATE;
            }

            // Shoot
            if(shootTimer <= 0 && shootAction.newPressed)
            {
                for(uint8_t i = 0; i < NB_BULLETS; i++) if(!bullets[i].sx)
                {
                    shootTimer = SHOOT_COOLDOWN;
                    bullets[i].sx = moveDirec;
                    bullets[i].x = characterX + CHARACTER_SIZE_X / 2.f;
                    bullets[i].y = characterY - SHOOT_HEIGHT;
                    break;
                }
            }
            shootTimer -= 1.f / FRAME_RATE;
            for(uint8_t i = 0; i < NB_BULLETS; i++) bullets[i].update();

            prevTime++;
        }

        // Draw
        window.clear(borderColor);
        window.draw(fieldRect);
        characterRect.setPosition(characterX * GAME_SCALE, GAME_SCALE * (characterY - CHARACTER_SIZE_Y));
        window.draw(characterRect);
        for(uint8_t i = 0; i < NB_BULLETS; i++) bullets[i].draw(window);

        // Draw texts
        text.setFillColor(defaultTextColor);
        if(libWrapper.getDeviceStatus(0) >= Lazynput::LibWrapper::DeviceStatus::UNSUPPORTED)
        {
            const Lazynput::Device &device = libWrapper.getDevice(0);
            text.setPosition(0, 0);
            sf::String deviceName(device.getName());
            if(libWrapper.getDeviceStatus(0) == Lazynput::LibWrapper::DeviceStatus::FALLBACK)
                deviceName += " [fallback]";
            if(!isSupported)
                deviceName += " [unsupported]";
            text.setString(deviceName);
            window.draw(text);
            float lineHeight = TEXT_SIZE * GAME_SCALE;
            bool hasDpad = isSupported ? device.hasInput("basic_gamepad.dpx"_hash) : libWrapper.getNumHat(0);
            bool hasJoystick = isSupported ? device.hasInput("basic_gamepad.lsx"_hash) : libWrapper.getNumAbs(0);
            text.setString("Move");
            text.setPosition(0, lineHeight);
            window.draw(text);
            std::string str;
            if(hasDpad) str += std::string("d-pad");
            if(hasDpad && hasJoystick) str += " or ";
            if(hasJoystick) str += "left joystick";
            if(!hasDpad && !hasJoystick) str += "nothing";
            text.setString(str);
            text.setPosition(INPUT_TEXT_X * GAME_SCALE, lineHeight);
            window.draw(text);
            for(uint8_t i = 0; i <= GameInput::MAX; i++)
            {
                text.setPosition(INPUT_TEXT_X * GAME_SCALE, (i + 2) * lineHeight);
                if(isSupported)
                {
                    if(!device.hasInput(inputMappings[i].hash)) continue;
                    Lazynput::LabelInfos labelInfos = device.getEnglishAsciiLabelInfos(inputMappings[i].hash);
                    text.setString(labelInfos.label);
                    if(labelInfos.hasColor)
                            text.setFillColor(sf::Color(labelInfos.color.r, labelInfos.color.g, labelInfos.color.b));
                    window.draw(text);
                }
                else
                {
                    if(inputMappings[i].button >= libWrapper.getNumBtn(0)) continue;
                    text.setString(std::string("B") + std::to_string(inputMappings[i].button + 1));
                    window.draw(text);
                }
                text.setFillColor(defaultTextColor);
                text.setString(inputMappings[i].name);
                text.setPosition(0, (i + 2) * lineHeight);
                window.draw(text);
            }
        }
        else
        {
            text.setString("No joystick detected");
            text.setPosition(0,0);
            window.draw(text);
        }
        if(paused)
        {
            text.setFillColor(defaultTextColor);
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
