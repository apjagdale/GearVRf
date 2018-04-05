/* Copyright 2015 Samsung Electronics Co., LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.gearvrf.io;


import org.gearvrf.GVRContext;
import org.gearvrf.GVRPerspectiveCamera;
import org.gearvrf.GVRScene;
import org.gearvrf.utility.Log;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.util.SparseArray;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;

/**
 * Use this class to translate MotionEvents generated by a mouse to manipulate
 * {@link GVRMouseController}s.
 */
final class GVRMouseDeviceManager {
    private static final String TAG = "GVRMouseDeviceManager";
    private static final String THREAD_NAME = "GVRMouseManagerThread";
    private EventHandlerThread thread;
    private SparseArray<GVRMouseController> controllers;
    private boolean threadStarted;


    GVRMouseDeviceManager(GVRContext context) {
        thread = new EventHandlerThread(THREAD_NAME);
        controllers = new SparseArray<>();
    }

    GVRCursorController getCursorController(GVRContext context, String name, int vendorId, int productId) {
        Log.d(TAG, "Creating Mouse Device");
        startThread();
        GVRMouseController controller = new GVRMouseController(context,
                GVRControllerType.MOUSE, name, vendorId, productId, this);
        int id = controller.getId();
        synchronized (controllers) {
            controllers.append(id, controller);
        }
        return controller;
    }

    void removeCursorController(GVRCursorController controller) {
        int id = controller.getId();
        synchronized (controllers) {
            controllers.remove(id);

            // stopThread the thread if no more devices are online
            if (controllers.size() == 0) {
                forceStopThread();
            }
        }
    }

    private static class GVRMouseController extends GVRCursorController
    {
        private static final KeyEvent BUTTON_1_DOWN = new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_BUTTON_1);
        private static final KeyEvent BUTTON_1_UP = new KeyEvent(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_BUTTON_1);

        private GVRMouseDeviceManager deviceManager;

        GVRMouseController(GVRContext context, GVRControllerType controllerType, String name, int
                vendorId, int productId, GVRMouseDeviceManager deviceManager) {
            super(context, controllerType, name, vendorId, productId);
            this.deviceManager = deviceManager;
            mConnected = true;
        }

        @Override
        public void setEnable(boolean flag) {
            if (mCursor != null)
            {
                mCursor.setEnable(flag);
            }
            if (!enable && flag) {
                enable = true;
                deviceManager.startThread();
                //set the enabled flag on the handler thread
                deviceManager.thread.setEnable(getId(), true);
                mConnected = true;
            } else if (enable && !flag) {
                enable = false;
                //set the disabled flag on the handler thread
                deviceManager.thread.setEnable(getId(), false);
                deviceManager.stopThread();
                mConnected = false;
                context.getInputManager().removeCursorController(this);
            }
        }

        @Override
        public void setScene(GVRScene scene) {
            if (!deviceManager.threadStarted) {
                super.setScene(scene);
            } else {
                deviceManager.thread.setScene(getId(), scene);
            }
        }

        void callParentSetEnable(boolean enable){
            super.setEnable(enable);
        }

        void callParentSetScene(GVRScene scene) {
            super.setScene(scene);
        }

        void callParentInvalidate() {
            super.invalidate();
        }

        @Override
        public void invalidate() {
            if (!deviceManager.threadStarted) {
                //do nothing
                return;
            }
            deviceManager.thread.sendInvalidate(getId());
        }

        @Override
        protected void setKeyEvent(KeyEvent keyEvent) {
            super.setKeyEvent(keyEvent);
        }

        @Override
        public synchronized boolean dispatchKeyEvent(KeyEvent event)
        {
            if (event.isFromSource(InputDevice.SOURCE_MOUSE))
            {
                if (deviceManager.thread.submitKeyEvent(getId(), event))
                {
                    return true;
                }
            }
            return false;
        }

        @Override
        public synchronized boolean dispatchMotionEvent(MotionEvent event)
        {
            if (event.isFromSource(InputDevice.SOURCE_MOUSE))
            {
                if (deviceManager.thread.submitMotionEvent(getId(), event))
                {
                    return true;
                }
            }
            return false;
        }

        private boolean processMouseEvent(float x, float y, float z, MotionEvent e)
        {
            if (scene == null)
            {
                return false;
            }
            float depth = mCursorDepth + z;

            if ((depth <= getNearDepth()) &&
                (depth >= getFarDepth()))
            {
                depth = mCursorDepth;
            }

            GVRPerspectiveCamera camera = scene.getMainCameraRig().getCenterCamera();
            float aspectRatio = camera.getAspectRatio();
            float fovY = camera.getFovY();
            float frustumHeightMultiplier = (float) Math.tan(Math.toRadians(fovY / 2)) * 2.0f;
            float frustumHeight = frustumHeightMultiplier * depth;
            float frustumWidth = frustumHeight * aspectRatio;
            int action = e.getAction();

            x = (frustumWidth * x) / 2.0f;
            y = (frustumHeight * y) / 2.0f;

            /*
             * The mouse does not report a key event against the primary
             * button click. Instead we generate a synthetic KeyEvent
             * against the mouse.
             */
            if (action == MotionEvent.ACTION_DOWN)
            {
                setKeyEvent(BUTTON_1_DOWN);
                if ((mTouchButtons & e.getButtonState()) != 0)
                {
                    setActive(true);
                }
            }
            else if (action == MotionEvent.ACTION_UP)
            {
                setKeyEvent(BUTTON_1_UP);
                setActive(false);
             }
            setMotionEvent(e);
            if (mCursorControl == CursorControl.CURSOR_DEPTH_FROM_CONTROLLER)
            {
                setCursorDepth(depth);
            }
            super.setPosition(x, y, -depth);
            return true;
        }
    }

    private class EventHandlerThread extends HandlerThread {
        private static final int MOTION_EVENT = 0;
        private static final int KEY_EVENT = 1;
        private static final int UPDATE_POSITION = 2;
        public static final int SET_ENABLE = 3;
        public static final int SET_SCENE = 4;
        public static final int SEND_INVALIDATE = 5;

        public static final int ENABLE = 0;
        public static final int DISABLE = 1;

        private Handler handler;

        EventHandlerThread(String name) {
            super(name);
        }

        void prepareHandler() {
            handler = new Handler(getLooper()) {
                @Override
                public void handleMessage(Message msg) {
                    int id = msg.arg1;
                    switch (msg.what) {
                        case MOTION_EVENT:
                            MotionEvent motionEvent = (MotionEvent) msg.obj;
                            if (dispatchMotionEvent(id, motionEvent) == false) {
                                // recycle if unhandled.
                                motionEvent.recycle();
                            }
                            break;
                        case KEY_EVENT:
                            KeyEvent keyEvent = (KeyEvent) msg.obj;
                            dispatchKeyEvent(id, keyEvent);
                            break;
                        case SET_ENABLE:
                            synchronized (controllers) {
                                final GVRMouseController c = controllers.get(id);
                                if (null != c) {
                                    c.callParentSetEnable(msg.arg2 == ENABLE);
                                }
                            }
                            break;

                        case SET_SCENE:
                            synchronized (controllers) {
                                final GVRMouseController c = controllers.get(id);
                                if (null != c) {
                                    c.callParentSetScene((GVRScene) msg.obj);
                                }
                            }
                            break;

                        case SEND_INVALIDATE:
                            synchronized (controllers) {
                                final GVRMouseController c = controllers.get(id);
                                if (null != c) {
                                    c.callParentInvalidate();
                                }
                            }
                            break;

                        default:
                            break;
                    }
                }
            };
        }

        boolean submitKeyEvent(int id, KeyEvent event) {
            if (threadStarted) {
                Message message = Message.obtain(null, KEY_EVENT, id, 0, event);
                return handler.sendMessage(message);
            }
            return false;
        }

        boolean submitMotionEvent(int id, MotionEvent event) {
            if (threadStarted) {
                MotionEvent clone = MotionEvent.obtain(event);
                Message message = Message.obtain(null, MOTION_EVENT, id, 0, clone);
                return handler.sendMessage(message);
            }
            return false;
        }

        void setEnable(int id, boolean enable) {
            if (threadStarted) {
                handler.removeMessages(SET_ENABLE);
                Message msg = Message.obtain(handler, SET_ENABLE, id, enable ? ENABLE : DISABLE);
                msg.sendToTarget();
            }
        }

        void setScene(int id, GVRScene scene){
            if (threadStarted) {
                handler.removeMessages(SET_SCENE);
                Message msg = Message.obtain(handler, SET_SCENE, id, 0, scene);
                msg.sendToTarget();
            }
        }

        void sendInvalidate(int id){
            if (threadStarted) {
                handler.removeMessages(SEND_INVALIDATE);
                Message msg = Message.obtain(handler, SEND_INVALIDATE, id, 0);
                msg.sendToTarget();
            }
        }

        private void dispatchKeyEvent(int id, KeyEvent event) {
            if (id != -1) {
                InputDevice device = event.getDevice();
                if (device != null) {
                    GVRMouseController mouseDevice = controllers.get(id);
                    mouseDevice.setKeyEvent(event);
                }
            }
        }

        // The following methods are taken from the controller sample on the
        // Android Developer web site:
        // https://developer.android.com/training/game-controllers/controller-input.html
        private boolean dispatchMotionEvent(int id, MotionEvent event) {
            InputDevice device = event.getDevice();
            if (id == -1 || device == null) {
                return false;
            }

            /*
             * Retrieve the normalized coordinates (-1 to 1) for any given (x,y)
             * value reported by the MotionEvent.
             */
            InputDevice.MotionRange range = device
                    .getMotionRange(MotionEvent.AXIS_X, event.getSource());
            float x = range.getMax() + 1;
            range = device.getMotionRange(MotionEvent.AXIS_Y, event.getSource());
            float y = range.getMax() + 1;
            float z = 0;
            if (event.getAction() == MotionEvent.ACTION_SCROLL)
            {
                z = (event.getAxisValue(MotionEvent.AXIS_VSCROLL) > 0 ? -1 : 1);
            }
            x = (event.getX() / x * 2.0f - 1.0f);
            y = 1.0f - event.getY() / y * 2.0f;

            GVRMouseController controller = controllers.get(id);
            return controller.processMouseEvent(x, y, z, event);
        }
    }

    void startThread(){
        if(!threadStarted){
            thread.start();
            thread.prepareHandler();
            threadStarted = true;
        }
    }

    void stopThread() {
        boolean foundEnabled = false;

        for(int i = 0 ;i< controllers.size(); i++){
            GVRCursorController controller = controllers.valueAt(i);
            if(controller.isEnabled()){
                foundEnabled = true;
                break;
            }
        }

        if (!foundEnabled && threadStarted) {
            thread.quitSafely();
            thread = new EventHandlerThread(THREAD_NAME);
            threadStarted = false;
        }
    }

    void forceStopThread(){
        if (threadStarted) {
            thread.quitSafely();
            thread = new EventHandlerThread(THREAD_NAME);
            threadStarted = false;
        }
    }
}