#include "multimouse.h"
#include "winusb/mousecore.h"
#include <cmath>

using namespace npnx;

MouseInstance::MouseInstance(MultiMouseSystem* parent, int hDevice) :
    mParent(parent),
    mDeviceHandle(hDevice)
{
    NPNX_ASSERT_LOG(parent, "MouseInstance no parent");

#define M_PI 3.141592654
    mRotateMatrix[0][0] = cos(M_PI);
    mRotateMatrix[0][1] = -sin(M_PI);
    mRotateMatrix[1][0] = -mRotateMatrix[0][1];
    mRotateMatrix[1][1] = mRotateMatrix[0][0];
#undef M_PI
}

void MouseInstance::SetMousePos(double xCoord, double yCoord) {
    std::cout << "SetMousePos: x, " << xCoord << "  y, " << yCoord << std::endl;
    //set mouse coord to origin, and then set the origin in screencoord.
    std::lock_guard<std::recursive_mutex> lck(objectMutex);
    mMousePosX = 0;
    mMousePosY = 0;
    mOriginInScreenCoordX = xCoord;
    mOriginInScreenCoordY = yCoord;
}

void MouseInstance::SetMouseAngle(double rotateInRad)
{
    //set mouse coord to origin, and then set the origin in screencoord.
    std::lock_guard<std::recursive_mutex> lck(objectMutex);
    mRotateMatrix[0][0] = cos(rotateInRad);
    mRotateMatrix[0][1] = -sin(rotateInRad);
    mRotateMatrix[1][0] = -mRotateMatrix[0][1];
    mRotateMatrix[1][1] = mRotateMatrix[0][0];
}

void MouseInstance::HandleReport(const MouseReport& report) {
    std::lock_guard<std::recursive_mutex> lck(objectMutex);
    if (report.xTrans != 0 || report.yTrans != 0) {
        lastMovementTime = std::chrono::high_resolution_clock::now();
        if (mParent->mEnableHCI && mParent->hciController.instances[mDeviceHandle]->messageType.load() != HCIMESSAGEINTYPE_POSITION) {
            mParent->hciController.instances[mDeviceHandle]->messageType = HCIMESSAGEINTYPE_POSITION;
            mParent->hciController.messageFifo.Push({ mDeviceHandle, HCIMESSAGEOUTTYPE_ANGLE_HALT, 0, 0 });
        }
    }
    mMousePosX += report.xTrans;
    mMousePosY += report.yTrans;
    if (report.button != mHidLastButton) {
        for (int i = 0; i < 7; i++) {
            if ((report.button & (1 << i)) ^ (mHidLastButton & (1 << i))) {
                double x, y;
                GetCursorPos(&x, &y);
                mParent->fifo.Push({ mDeviceHandle, 1 << i, (report.button & (1 << i)) >> i, x, y });
            }
        }
        mHidLastButton = report.button;
    }
}

void MouseInstance::SetLastPush(uint8_t button, int action, double screenX, double screenY) {
    // I think lastpush info is written and read only by main thread (set is from fifo.)
    // std::lock_guard<std::recursive_mutex> lck(objectMutex);
    if (action == GLFW_PRESS) { mPushedButton |= button; }
    else { mPushedButton &= (~button); }
    mLastPushPosInScreenX = screenX;
    mLastPushPosInScreenY = screenY;
}

void MouseInstance::GetLastPush(uint8_t* button, double* screenX, double* screenY) {
    // std::lock_guard<std::recursive_mutex> lck(objectMutex);
    *button = mPushedButton;
    *screenX = mLastPushPosInScreenX;
    *screenY = mLastPushPosInScreenY;
}

void MouseInstance::GetCursorPos(double* x, double* y, bool bGetMouseCenterPos) {
    std::lock_guard<std::recursive_mutex> lck(objectMutex);

    int mouseX = mMousePosX;
    int mouseY = mMousePosY;

    if (!bGetMouseCenterPos) {
        mouseY += 270 / mParent->mSensitivityY;  //  the cursor is outside the mouse. 
    }

    *x = mouseX * mRotateMatrix[0][0] + mouseY * mRotateMatrix[0][1];
    *x *= mParent->mSensitivityX;
    *x += mOriginInScreenCoordX;
    *x = (*x - WINDOW_WIDTH / 2) / (WINDOW_WIDTH / 2);

    *y = mouseX * mRotateMatrix[1][0] + mouseY * mRotateMatrix[1][1];
    *y *= mParent->mSensitivityY;
    *y += mOriginInScreenCoordY;
    *y = -(*y - WINDOW_HEIGHT / 2) / (WINDOW_HEIGHT / 2);
}

double MouseInstance::NoMovementTimer() {
    std::lock_guard<std::recursive_mutex> lck(objectMutex);
    return std::chrono::duration_cast<std::chrono::duration<double>> (std::chrono::high_resolution_clock::now() - lastMovementTime).count();
}


MultiMouseSystem::MultiMouseSystem() {
    mouses.clear();
}

MultiMouseSystem::~MultiMouseSystem() {
    for (auto iter : mouses) {
        if (iter) {
            delete iter;
        }
    }
    mouses.clear();
}

void MultiMouseSystem::Init(MOUSEBUTTONCALLBACKFUNC func, bool enableHCI)
{
    NPNX_ASSERT(!mInitialized);
    mEnableHCI = enableHCI;
    mouseButtonCallback = func;
    num_mouse = core.Init(default_vid, default_vid, [&, this](int idx, MouseReport report) -> void {this->reportCallback(idx, report); });
    if (mEnableHCI) {
        hciController.Init(this, num_mouse);
    }
    mInitialized = true;
}

void MultiMouseSystem::GetCursorPos(int hDevice, double* x, double* y, bool bGetMouseCenterPos) {
    if (num_mouse == 0) {
        *x = 0;
        *y = 0;
        return;
    }
    else {
        NPNX_ASSERT(hDevice < num_mouse);
        mouses[hDevice]->GetCursorPos(x, y, bGetMouseCenterPos);
    }
}

void MultiMouseSystem::RegisterMouseRenderer(Renderer* renderer, std::function<bool(int)> defaultVisibleFunc) {
    NPNX_ASSERT(!mRenderered);
    NPNX_ASSERT(mInitialized);
    NPNX_ASSERT(postMouseRenderer);
    mouseRenderer = renderer;
    originalMouseVisibleFunc = defaultVisibleFunc;
    mRenderered = true;

    for (int i = 0; i < num_mouse; i++) {
        checkNewMouse(i);
    }
    core.Start();

    if (mEnableHCI) {
        hciController.Start();
    }
}

void MultiMouseSystem::checkNewMouse(int hDevice)
{
    mouses.push_back(new MouseInstance(this, hDevice));
    LayerObject* cursorLayer = mouseRenderer->mLayers[*(float*)&hDevice];
    cursorLayer->beforeDraw = [=](int nbFrames) {
        double x, y;
        this->GetCursorPos(hDevice, &x, &y);
        glUniform1f(glGetUniformLocation(cursorLayer->mParent->mDefaultShader->mShader, "xTrans"), x);
        glUniform1f(glGetUniformLocation(cursorLayer->mParent->mDefaultShader->mShader, "yTrans"), y);
        return 0;
    };
    cursorLayer->afterDraw = [=](int nbFrames) {
        glUniform1f(glGetUniformLocation(cursorLayer->mParent->mDefaultShader->mShader, "xTrans"), 0.0f);
        glUniform1f(glGetUniformLocation(cursorLayer->mParent->mDefaultShader->mShader, "yTrans"), 0.0f);
        return 0;
    };
    cursorLayer->visibleCallback = originalMouseVisibleFunc;
}

void MultiMouseSystem::PollMouseEvents() {
    MouseFifoReport report;
    while (fifo.Pop(&report)) {
        mouseButtonCallback(report.hDevice, report.button, report.action, report.screenX, report.screenY);
    }

    if (mEnableHCI) {
        HCIMessageReport hciReport;
        while (hciController.messageFifo.Pop(&hciReport)) {
            if (this->hciController.instances[hciReport.index]->tokenReport.isCompleted) {
                // wait last report to be consumed
                continue;
            }

            switch (hciReport.type) {
            case HCIMESSAGEOUTTYPE_POSITION:
            {
                double x, y;
                hciToScreenPosFunc(hciReport.param1, hciReport.param2, &x, &y);
                mouses[hciReport.index]->SetMousePos(x, y);
                mouseButtonCallback(hciReport.index, 0xffffffff, GLFW_PRESS, (double)hciReport.param1, (double)hciReport.param2);
                this->hciController.instances[hciReport.index]->tokenReport.y = hciReport.param1;
                this->hciController.instances[hciReport.index]->tokenReport.x = hciReport.param2;

                if (mEnableAngle) {
                    RectLayer* targetLayer = dynamic_cast<RectLayer*> (postMouseRenderer->mLayers[*(float*)&hciReport.index]);
                    NPNX_ASSERT(targetLayer);
                    targetLayer->visibleCallback = [](int) {return true; };
                    targetLayer->beforeDraw = [&, hciReport, targetLayer](int) {
                        double x, y;
                        this->GetCursorPos(hciReport.index, &x, &y, true);
                        glUniform1f(glGetUniformLocation(targetLayer->mParent->mDefaultShader->mShader, "xTrans"), x);
                        glUniform1f(glGetUniformLocation(targetLayer->mParent->mDefaultShader->mShader, "yTrans"), y);
                        return 0;
                    };
                    targetLayer->afterDraw = [&, targetLayer](int) {
                        glUniform1f(glGetUniformLocation(targetLayer->mParent->mDefaultShader->mShader, "xTrans"), 0.0f);
                        glUniform1f(glGetUniformLocation(targetLayer->mParent->mDefaultShader->mShader, "yTrans"), 0.0f);
                        return 0;
                    };
                    targetLayer->textureNoCallback = [](const int) -> unsigned int {return 0; };
                    hciController.instances[hciReport.index]->messageType = HCIMESSAGEINTYPE_ANGLE_1;
                }
            }
            break;
            case HCIMESSAGEOUTTYPE_ANGLE:
            {
                float angleInRad = *(float*)&hciReport.param1;
                //NPNX_LOG(angleInRad / 3.1415926535897f);
                mouses[hciReport.index]->SetMouseAngle(angleInRad);
                this->hciController.instances[hciReport.index]->tokenReport.angle = *(int32_t*)&hciReport.param2;
                this->hciController.instances[hciReport.index]->tokenReport.isCompleted = true;
                RectLayer* targetLayer = dynamic_cast<RectLayer*> (postMouseRenderer->mLayers[*(float*)&hciReport.index]);
                NPNX_ASSERT(targetLayer);
                targetLayer->visibleCallback = [](int) {return false; };
                hciController.instances[hciReport.index]->messageType = HCIMESSAGEINTYPE_POSITION;
            }
            break;
            case HCIMESSAGEOUTTYPE_ANGLE_SIGNAL:
            {
                if (hciController.instances[hciReport.index]->messageType.load() == HCIMESSAGEINTYPE_ANGLE_1) {
                    RectLayer* targetLayer = dynamic_cast<RectLayer*> (postMouseRenderer->mLayers[*(float*)&hciReport.index]);
                    NPNX_ASSERT(targetLayer);
                    targetLayer->textureNoCallback = [](const int) -> unsigned int {return 1; };
                    hciController.instances[hciReport.index]->messageType = HCIMESSAGEINTYPE_ANGLE_2;
                }
            }
            break;
            case HCIMESSAGEOUTTYPE_ANGLE_HALT:
            {
                RectLayer* targetLayer = dynamic_cast<RectLayer*> (postMouseRenderer->mLayers[*(float*)&hciReport.index]);
                NPNX_ASSERT(targetLayer);
                targetLayer->visibleCallback = [](int) {return false; };
                // MESSAGEINTYPE is set by mouse movement report handler, do not do it again.
            }
            break;
            }
        }
    }
}

//this must be thread safe for diffrent mouse.
void MultiMouseSystem::reportCallback(int hDevice, MouseReport report) {
    mouses[hDevice]->HandleReport(report);
}

TokenReport MultiMouseSystem::GetTokenReportByIndex(int index) {
    if (index >= this->hciController.mNumMouse) {
        return TokenReport();
    }
    return this->hciController.instances[index]->tokenReport;
}

void MultiMouseSystem::ClearTokenReportByIndex(int index) {
    if (index < this->hciController.mNumMouse) {
        this->hciController.instances[index]->tokenReport.isCompleted = false;
    }
}

void MultiMouseSystem::Stop() {
    core.Stop();
    hciController.Stop();
}

namespace npnx {

    MultiMouseSystem multiMouseSystem;

}