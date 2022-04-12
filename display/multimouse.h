#ifndef DISPLAY_MULTIMOUSE_H_
#define DISPLAY_MULTIMOUSE_H_

#include "common.h"
#include "layerobject.h"
#include "renderer.h"
#include "mousefifo.h"
#include "winusb/mousecore.h"
#include "hcicontroller.h"

#include <mutex>
#include <functional>
#include <map>
#include <chrono>

// mouse part 
// Mouse data is read by render thread, written by mousecore thread and HCI thread.
//
// The tricky point is mouse moving and button events will cause the renderer to change.
// Renderer part is hard to keep thread safe.
// 
// Here we use a fifo (multiMouseSystem.mouseFifo) to collect all button events
// Main thread must call PollMouseEvents every frame to process it.
//
// The moving information is not in the fifo, because it is too many to handle in one thread.
// Any time we need the position of a mouse should use GetCursorPos.
//
// For HCI info, 
// the HCI thread should directly SetMouseState, because it changes the position of mouse. 
// and send the fifo a button push report (button bit is arbitrary, but should be one of the first 3 bit).
// HCI must give a critiria for PUSH and RELEASE, and fill the fifo report with GetCursorPos.


namespace npnx {

typedef std::function<void(int, int, double *, double *)> HCITOSCREENPOSFUNC_T;

class MultiMouseSystem;

//TODO: this class must be thread safe.
class MouseInstance {
public:
  MouseInstance(MultiMouseSystem *parent, int hDevice);
  ~MouseInstance() = default;

  // this is for HCI message.
  // the rotate angle is the right-to-top angle in radian.
  //              |     /
  //              | a /
  //              |--/
  //              |/
  // ----------------------------
  //              |
  //              |
  //              |
  //              |
  //
  void SetMouseAngle(double rotate);
  
  void SetMousePos(double xCoord, double yCoord); 
  
  void HandleReport(const MouseReport & report);

  void SetLastPush(uint8_t button, int action, double screenX, double screenY);

  void GetLastPush(uint8_t *button, double *screenX, double *screenY);

  void GetCursorPos(double *x, double *y, bool bGetMouseCenterPos = false);

  double NoMovementTimer();

private:
  std::recursive_mutex objectMutex;
  MultiMouseSystem *mParent = NULL;
  int mDeviceHandle = 0;
  
  RectLayer *cursorLayer;
  
  int mMousePosX = 0, mMousePosY = 0; // the mouse space coordinate
  uint8_t mHidLastButton = 0x00; //for handle hid report.

  //we want to know last press position on screen.                                    
  double mLastPushPosInScreenX = 0.0, mLastPushPosInScreenY = 0.0; 
  //this is for fifo and main thread (render thread).
  uint8_t mPushedButton = 0x00;
  
  //this is for transform from mouse input to screen
  //every time we get a rotate angle, we calc the matrix.
  double mRotateMatrix[2][2]; 
  double mOriginInScreenCoordX = WINDOW_WIDTH / 2, mOriginInScreenCoordY = WINDOW_HEIGHT / 2;

  std::chrono::high_resolution_clock::time_point lastMovementTime;

};

typedef void (*MOUSEBUTTONCALLBACKFUNC)(int hDevice, int button, int action, double ScreenX, double ScreenY);

class MultiMouseSystem {
public:
   MultiMouseSystem(); // if enableHCI == false, We handle moving and button event as a normal mouse.
  ~MultiMouseSystem();

  void Init(MOUSEBUTTONCALLBACKFUNC func, bool enableHCI = true); //register raw input for mouse equipments;

  inline void RegisterPoseMouseRenderer(Renderer* renderer) {
    postMouseRenderer = renderer;
  } 
  
  //renderer should have cNumLimit Rectlayers. 
  void RegisterMouseRenderer(Renderer* renderer, std::function<bool(int)> defaultVisibleFunc);


  void GetCursorPos(int hDevice, double *x, double *y, bool bGetMouseCenterPos = false);

  void PollMouseEvents();

  HCIReport MultiMouseSystem::GetHCIReportByIndex(int index);
  void MultiMouseSystem::ClearHCIReportByIndex(int index);

private:
  void checkNewMouse(int hDevice);  
  void reportCallback(int hDevice, MouseReport report);

public:
  static const size_t cNumLimit = NUM_MOUSE_MAXIMUM;
  std::vector<MouseInstance *> mouses;
  inline size_t GetNumMouse() { return mouses.size(); }
  double mSensitivityX = 0.091, mSensitivityY = 0.091;
  bool mEnableAngle = true;
  MOUSEBUTTONCALLBACKFUNC mouseButtonCallback;
  MouseFifo fifo;
  MouseCore core;
  HCIController hciController;
  bool mEnableHCI;
  HCITOSCREENPOSFUNC_T hciToScreenPosFunc = [] (int p1, int p2, double *sx, double *sy){
    // (32,0)
    //  |
    // (0,0) -- (0,32)
    *sx = p2 * 32 + 420;
    *sy = WINDOW_HEIGHT - p1 * 32;
  };

private:
  Renderer *mouseRenderer;
  Renderer *postMouseRenderer = NULL;
  std::function<bool(int)> originalMouseVisibleFunc;


  int num_mouse = 0;
  bool mInitialized = false;
  bool mRenderered = false;
};

extern MultiMouseSystem multiMouseSystem;

} // namespace npnx
#endif // !DISPLAY_MULTIMOUSE_H_