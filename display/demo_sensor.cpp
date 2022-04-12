#include "common.h"

#include <string>
#include <cstdlib>
#include <chrono>
#include <cstdio>

#include "imageUtils.h"
#include "shader.h"
#include "renderer.h"
#include "layerobject.h"
#include "textrectlayer.h"
#include "multimouse.h"

using namespace npnx;

const int num_position_texture = 248; // 248
const int mod_position_texture = 64; // 64
int image_shift = 0;
const int feedbackLength = 240;

class Token_ColorWidget {
public:
    bool showing = true;
    int nextBeginFrame = -1;
    TextRectLayer* target;
    // When put the token on screen first time,
    // the angle is set as 0
    // and the R/G/B value is set as 128
    // although the token may be put with bias, 
    // the angle offset is set as the placement angle of first place
    int value;
    int valueStep;
    float lastAngle; // the real angle of sensor and screen
    float angle; // the adjusted angle relate to init placement angle
    bool isInitialized = false;
    int Left, Right, Bottom, Top;

    Token_ColorWidget() : value(128), valueStep(10), angle(0), lastAngle(0) {}

    Token_ColorWidget(float initAngle) {
        Initialize(initAngle, 128);
    }

    Token_ColorWidget(float initAngle, int initValue) {
        Initialize(initAngle, initValue);
    }

    void Initialize(float initAngle, int initValue) {
        this->valueStep = 10; // 0, 10, ..., 250(, 255)
        this->value = initValue;
        this->lastAngle = initAngle;
        this->angle = 0.0f;
        this->isInitialized = true;
    }

    void InitializeTarget(TextRectLayer* target) {
        this->target = target;
        this->target->content = std::to_string(this->value);
    }

    void Update(float angle) {
        if (!this->isInitialized) {
            Initialize(angle, 128);
            return;
        }
        float addAngle = 0;
        float tmpAngle = angle;
        if (abs(tmpAngle - this->lastAngle) > 180) {
            tmpAngle += this->lastAngle > 180 ? 360 : -360;
        }
        addAngle = tmpAngle - this->lastAngle;
        if (UpdateWithAddAngle(addAngle)) {
            this->lastAngle = angle;
        }
    }

    // return: true->updated, false->have not updated
    bool UpdateWithAddAngle(float addAngle) {
        // addAngle to RGB value
        float addRGB = addAngle / 180 * 128;
        // number of RGB value steps
        int NSteps = 0;
        if (addRGB > 0) {
            NSteps = (int)((addRGB + this->valueStep / 2) / this->valueStep);
        }
        else if (addRGB < 0) {
            NSteps = (int)((addRGB - this->valueStep / 2) / this->valueStep);
        }

        // if NSteps is 0, do not update and wait to next large angle change
        // this can avoid unstablility due to error of angle estimation
        if (NSteps != 0) {
            std::cout << this->lastAngle << ", addAAngle: " << addAngle << ", addRGB: " << addRGB << ", NSteps: " << NSteps << ", value : " << this->value;
            float tmpVal = this->angle + addAngle;
            this->angle = NSteps > 0 ? min(180.0f, tmpVal) : max(-180.0f, tmpVal);
            tmpVal = (this->angle + 180) / 360 * 255;
            if (tmpVal < 255) {
                tmpVal = (int)(tmpVal / 10) * 10;
            }
            // update R/G/B value
            this->value = NSteps > 0 ? min(255.0f, tmpVal) : max(0.0f, tmpVal);
            std::cout << " to " << this->value << ", Angle: " << this->angle << std::endl;
            // update content of R/G/B textlayer
            target->content = std::to_string(this->value);
            return true;
        }
        return false;
    }

    void UpdateValidVLCIndex(int Left, int Right, int Bottom, int Top) {
        this->Left = Left;
        this->Right = Right;
        this->Bottom = Bottom;
        this->Top = Top;
    }

    bool IsInside(int x, int y) {
        return x >= this->Left && x <= this->Right 
            && y >= this->Bottom && y <= this->Top;
    }
};

class Test_ColorWidget {
public:
    GLFWwindow* window;
    Token_ColorWidget* tokens; // 0/1/2 -> R/G/B
    int bgIndex = 0;
    int nbFrames = 0;
    bool running = false;

    Test_ColorWidget(GLFWwindow* window, Token_ColorWidget* tokens) : window(window), tokens(tokens) {}

    void Update(int x, int y, float angle) {
        for (int i = 0; i < 3; i++) {
            if (tokens[i].IsInside(x, y)) {
                tokens[i].Update(angle);
            }
        }
    }

    void Update(HCIReport hciReport) {
        if (hciReport.isCompleted) {
            for (int i = 0; i < 3; i++) {
                if (tokens[i].IsInside(hciReport.x, hciReport.y)) {
                    tokens[i].Update(hciReport.angle);
                }
            }
        }
    }
};

auto truefunc = [] (int) -> bool {return true;};
auto falsefunc = [] (int) -> bool {return false;};

void before_every_frame() {}

void mouse_button_callback(int hDevice, int button, int action, double screenX, double screenY) {}

void InitGLFWWindow(GLFWwindow*& window) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);

    int monitorCount;
    GLFWmonitor** pMonitor = glfwGetMonitors(&monitorCount);

    int holographic_screen = -1;
    for (int i = 0; i < monitorCount; i++) {
        int screen_x, screen_y;
        const GLFWvidmode* mode = glfwGetVideoMode(pMonitor[i]);
        screen_x = mode->width;
        screen_y = mode->height;
        std::cout << "Screen size is X = " << screen_x << ", Y = " << screen_y << std::endl;
        if (screen_x == WINDOW_WIDTH && screen_y == WINDOW_HEIGHT) {
            holographic_screen = i;
        }
    }
    NPNX_LOG(holographic_screen);


#if (defined __linux__ || defined NPNX_BENCHMARK)
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "My Title", NULL, NULL);

#else
    if (holographic_screen == -1)
        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "My Title", NULL, NULL);
    else
        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Holographic projection", pMonitor[holographic_screen], NULL);
#endif  

    NPNX_ASSERT(window);
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    NPNX_ASSERT(!err);

    if (wglewIsSupported("WGL_EXT_swap_control"))
    {
        printf("OK, we can use WGL_EXT_swap_control\n");
    }
    else
    {
        printf("[WARNING] WGL_EXT_swap_control is NOT supported.\n");
    }
    wglSwapIntervalEXT(1);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

    
}

void InitDefaultShader(Shader& defaultShader) {
    defaultShader.LoadShader(NPNX_FETCH_DATA("defaultVertex.glsl"), NPNX_FETCH_DATA("defaultFragment.glsl"));
    defaultShader.Use();
    glUniform1i(glGetUniformLocation(defaultShader.mShader, "texture0"), 0);
    glUniform1f(glGetUniformLocation(defaultShader.mShader, "xTrans"), 0.0f);
    glUniform1f(glGetUniformLocation(defaultShader.mShader, "yTrans"), 0.0f);
    glUniform1i(glGetUniformLocation(defaultShader.mShader, "isSpecified"), 0);
}

void InitAdjustShader(Shader& adjustShader) {
    adjustShader.LoadShader(NPNX_FETCH_DATA("defaultVertex.glsl"), NPNX_FETCH_DATA("adjustFragment.glsl"));
    adjustShader.Use();
    glUniform1i(glGetUniformLocation(adjustShader.mShader, "texture0"), 0);
    glUniform1f(glGetUniformLocation(adjustShader.mShader, "xTrans"), 0.0f);
    glUniform1f(glGetUniformLocation(adjustShader.mShader, "yTrans"), 0.0f);
    glUniform1i(glGetUniformLocation(adjustShader.mShader, "centrosymmetric"), 0);
    glUniform1i(glGetUniformLocation(adjustShader.mShader, "rawScreen"), 1);
    glUniform1i(glGetUniformLocation(adjustShader.mShader, "letThrough"), 0);
}

int main()
{
    // this guarantee that the random target rectangle will be same every time we run.
    srand(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()); 
    NPNX_LOG(NPNX_DATA_PATH);

    GLFWwindow* window;
    InitGLFWWindow(window);

    Token_ColorWidget token_[3]; // 0/1/2 -> R/G/B token
    for (int i = 0; i < 3; i++) {
        token_[i] = Token_ColorWidget();
    }

    Test_ColorWidget test_(window, token_);

    // load shaders
    Shader defaultShader;
    InitDefaultShader(defaultShader);
    Shader adjustShader;
    InitAdjustShader(adjustShader);
    
    unsigned int fbo0, fboColorTex0;
    generateFBO(fbo0, fboColorTex0);

    // Renderer of background and text for RGB values
    Renderer renderer(&defaultShader, fbo0);
    RectLayer bg(-1.0f, -1.0f, 1.0f, 1.0f, -999.0f);
    bg.mTexture.push_back(makeTextureFromImage(NPNX_FETCH_DATA("tune_bg.png")));
    bg.textureNoCallback = [&](int _) {return 0; };
    renderer.AddLayer(&bg);
    const float textVSize = 0.5f;
    const float textHSize = textVSize / 2.0f ;
    TextRectLayer text[3];
    text[0] = TextRectLayer(-textHSize / 2 + -0.01f, -textVSize / 2 + -0.17f, textHSize / 2 + -0.01f, textVSize / 2 + -0.17f, -12.0f);
    renderer.AddLayer(&text[0]);
    text[1] = TextRectLayer(-textHSize / 2 + 0.55f, -textVSize / 2 + 0.79f, textHSize / 2 + 0.55f, textVSize / 2 + 0.79f, -11.0f);
    renderer.AddLayer(&text[1]);
    text[2] = TextRectLayer(-textHSize / 2 + 0.55f, -textVSize / 2 + -0.17f, textHSize / 2 + 0.55f, textVSize / 2 + -0.17f, -13.0f);
    renderer.AddLayer(&text[2]);
    renderer.Initialize();

    // bind R/G/B text layer to R/G/B token
    for (int i = 0; i < 3; i++) {
        token_[i].InitializeTarget(&text[i]);
    }
    // Set Valid VLC Index: Left, Right, Bottom, Top
    token_[0].UpdateValidVLCIndex(5, 10, 4, 9);
    token_[1].UpdateValidVLCIndex(22, 27, 4, 9);
    token_[2].UpdateValidVLCIndex(22, 27, 20, 25);

    // renderer for board showing the color
    Renderer colorBoardRenderer(&defaultShader, 0);
    RectLayer colorBoard(-1.0f, -1.0f, -3.0f / 16.0f, 1.0f, 1.0f);
    colorBoard.mTexture.push_back(makeTextureFromImage(NPNX_FETCH_DATA("whiteblock.png")));
    colorBoard.visibleCallback = [](int) {return true; };
    colorBoard.textureNoCallback = [&](int _) {return 0; };
    colorBoardRenderer.AddLayer(&colorBoard);
    colorBoardRenderer.Initialize();

    // renderer for VLC signals
    Renderer postRenderer(&adjustShader, 0);
    postRenderer.mDefaultTexture.assign({ 0, fboColorTex0 });
    RectLayer postBaseRect(-1.0, -1.0, 1.0, 1.0, -999.9);
    postBaseRect.mTexture.push_back(0);
    postBaseRect.beforeDraw = [&](const int nbFrames) {
        glUniform1i(glGetUniformLocation(postRenderer.mDefaultShader->mShader, "letThrough"), 1);
        return 0;
    };
    postBaseRect.afterDraw = [&](const int nbFrames) {
        glUniform1i(glGetUniformLocation(postRenderer.mDefaultShader->mShader, "letThrough"), 0);
        return 0;
    };
    postRenderer.AddLayer(&postBaseRect);
    RectLayer postRect(-3.0f / 16.0f, -1.0f, 15.0f / 16.0f, 1.0f, 999.9f);
    for (int i = 0; i < num_position_texture; i++) {
        std::string pos_texture_path = "fremw2_";
        pos_texture_path += std::to_string(i);
        pos_texture_path += ".png";
        postRect.mTexture.push_back(makeTextureFromImage(NPNX_FETCH_DATA(pos_texture_path)));
    }
    postRect.visibleCallback = [](int) {return true; };
    postRect.textureNoCallback = [=](int nbFrames) {return nbFrames % mod_position_texture + image_shift; };
    postRenderer.AddLayer(&postRect);
    postRenderer.Initialize();

    // renderer for angle estimation
    Renderer postMouseRenderer(&defaultShader, 0);
    GLuint mouseWhiteBlockTex = makeTextureFromImage(NPNX_FETCH_DATA("whiteblock.png"));
    GLuint mouseRedBlockTex = makeTextureFromImage(NPNX_FETCH_DATA("redblock.png"));
    for (int i = 0; i < multiMouseSystem.cNumLimit; i++) {
        const float blockVSize = 0.15f;
        const float blockHSize = blockVSize * WINDOW_HEIGHT / WINDOW_WIDTH;
        RectLayer* postColor = new RectLayer(-blockHSize / 2, -blockVSize / 2, blockHSize / 2, blockVSize / 2, *(float*)&i);
        postColor->mTexture.push_back(mouseRedBlockTex);
        postColor->mTexture.push_back(mouseWhiteBlockTex);
        postColor->visibleCallback = [](int) {return false; };
        postMouseRenderer.AddLayer(postColor);
    }
    postMouseRenderer.Initialize();

    multiMouseSystem.Init(mouse_button_callback, true);
    multiMouseSystem.RegisterPoseMouseRenderer(&postMouseRenderer);
    multiMouseSystem.mEnableAngle = false;

    test_.nbFrames = 0;
    int lastNbFrames = 0;
    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window))
    {
        //before_every_frame();

        renderer.Draw(test_.nbFrames);
        postRenderer.Draw(test_.nbFrames);
        postMouseRenderer.Draw(test_.nbFrames);
        
        defaultShader.Use();
        glUniform1i(glGetUniformLocation(defaultShader.mShader, "isSpecified"), 1);
        float color_x = token_[0].value / 255.0f;
        float color_y = token_[1].value / 255.0f;
        float color_z = token_[2].value / 255.0f;
        glUniform3f(glGetUniformLocation(defaultShader.mShader, "specifyColor"), color_x, color_y, color_z);
        colorBoardRenderer.Draw(test_.nbFrames);
        glUniform1i(glGetUniformLocation(defaultShader.mShader, "isSpecified"), 0);

        test_.nbFrames++;
        double thisTime = glfwGetTime();
        double deltaTime = thisTime - lastTime;
        if (deltaTime > 1.0)
        {
            glfwSetWindowTitle(window, std::to_string((test_.nbFrames - lastNbFrames) / deltaTime).c_str());
            lastNbFrames = test_.nbFrames;
            lastTime = thisTime;

            // For testing function of color widge
            /*int validX[] = { 5,6,7,8,9,10,22,23,24,25,26,27 };
            int validY[] = { 4,5,6,7,8,9,20,21,22,23,24,25 };
            test_.Update(validX[rand() % 12], validY[rand() % 12], rand() % 360);*/
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
        multiMouseSystem.PollMouseEvents();
        for (int i = 0; i < 3; i++) {
            test_.Update(multiMouseSystem.GetHCIReportByIndex(i));
            multiMouseSystem.ClearHCIReportByIndex(i);
        }
    }
    postMouseRenderer.FreeLayers();
    return 0;

}