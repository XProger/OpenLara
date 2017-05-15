package org.xproger.openlara;

import java.util.ArrayList;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import android.content.pm.PackageManager;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.Renderer;
import android.os.Bundle;
import android.app.Activity;
import android.content.res.AssetFileDescriptor;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnGenericMotionListener;
import android.view.View.OnKeyListener;
import android.view.View.OnTouchListener;
import android.view.Window;
import android.view.WindowManager;
//import android.util.Log;

public class MainActivity extends Activity implements OnTouchListener, OnKeyListener, OnGenericMotionListener, SensorEventListener {
    private Wrapper wrapper;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON |
                WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD |
                WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED |
                WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON);

        super.onCreate(savedInstanceState);

        GLSurfaceView view = new GLSurfaceView(this);
        view.setEGLContextClientVersion(2);
        view.setEGLConfigChooser(8, 8, 8, 8, 16, 8);
        view.setPreserveEGLContextOnPause(true);
        view.setRenderer(wrapper = new Wrapper());

        view.setFocusable(true);
        view.setFocusableInTouchMode(true);

        view.setOnTouchListener(this);
        view.setOnGenericMotionListener(this);
        view.setOnKeyListener(this);
        //setAsyncReprojectionEnabled(true);
        //setSustainedPerformanceMode(this, true);
        setContentView(view);
/*
        SensorManager sm = (SensorManager)getSystemService(SENSOR_SERVICE);
        sm.registerListener(this, sm.getDefaultSensor(Sensor.TYPE_GAME_ROTATION_VECTOR), SensorManager.SENSOR_DELAY_FASTEST);
*/
        try {
            String packName = getPackageManager().getPackageInfo(getPackageName(), PackageManager.GET_ACTIVITIES).applicationInfo.sourceDir;
            // hardcoded demo level and music
            AssetFileDescriptor fLevel = this.getResources().openRawResourceFd(R.raw.level2);
            AssetFileDescriptor fMusic = this.getResources().openRawResourceFd(R.raw.music);

            wrapper.onCreate(packName, getCacheDir().getAbsolutePath() + "/", (int)fLevel.getStartOffset(), (int)fMusic.getStartOffset());
        } catch (Exception e) {
            e.printStackTrace();
            finish();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        wrapper.onDestroy();
    }

    @Override
    protected void onPause() {
        super.onPause();
        wrapper.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        wrapper.onResume();
    }

    @Override
    public boolean onTouch(View v, MotionEvent event) {
        int action = event.getAction();
        int type = action & MotionEvent.ACTION_MASK;
        int state;

        switch (type) {
            case MotionEvent.ACTION_DOWN :
            case MotionEvent.ACTION_UP :
            case MotionEvent.ACTION_MOVE :
                state = type == MotionEvent.ACTION_MOVE ? 3 : (type == MotionEvent.ACTION_DOWN ? 2 : 1);
                for (int i = 0; i < event.getPointerCount(); i++)
                    wrapper.onTouch(event.getPointerId(i), state, event.getX(i), event.getY(i));
                break;
            case MotionEvent.ACTION_POINTER_DOWN :
            case MotionEvent.ACTION_POINTER_UP :
                int i = (action & MotionEvent.ACTION_POINTER_INDEX_MASK) >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
                state = type == MotionEvent.ACTION_POINTER_DOWN ? 2 : 1;
                wrapper.onTouch(event.getPointerId(i), state, event.getX(i), event.getY(i));
                break;
        }
        return true;
    }

    @Override
    public void onAccuracyChanged(Sensor arg0, int arg1) {
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        wrapper.onTouch(-100, 0, -event.values[1], event.values[0]);
        wrapper.onTouch(-100, 1,  event.values[2], event.values[3]);
    }

    @Override
    public boolean onGenericMotion(View v, MotionEvent event) {
        int src = event.getDevice().getSources();

        boolean isMouse = (src & (InputDevice.SOURCE_MOUSE)) != 0;
        boolean isJoy   = (src & (InputDevice.SOURCE_GAMEPAD | InputDevice.SOURCE_JOYSTICK)) != 0;

        if (isMouse) {
            return true;
        }

        if (isJoy) {
            wrapper.onTouch(0, -3, event.getAxisValue(MotionEvent.AXIS_X),
                    event.getAxisValue(MotionEvent.AXIS_Y));

            wrapper.onTouch(0, -4, event.getAxisValue(MotionEvent.AXIS_Z),
                    event.getAxisValue(MotionEvent.AXIS_RZ));

            wrapper.onTouch(0, -5, event.getAxisValue(MotionEvent.AXIS_HAT_X),
                    event.getAxisValue(MotionEvent.AXIS_HAT_Y));
        }

        return true;
    }

    @Override
    public boolean onKey(View v, int keyCode, KeyEvent event) {
        int btn;

        switch (keyCode) {
            case KeyEvent.KEYCODE_BUTTON_A      : btn = -0;  break;
            case KeyEvent.KEYCODE_BUTTON_B      : btn = -1;  break;
            case KeyEvent.KEYCODE_BUTTON_X      : btn = -2;  break;
            case KeyEvent.KEYCODE_BUTTON_Y      : btn = -3;  break;
            case KeyEvent.KEYCODE_BUTTON_L1     : btn = -4;  break;
            case KeyEvent.KEYCODE_BUTTON_R1     : btn = -5;  break;
            case KeyEvent.KEYCODE_BUTTON_SELECT : btn = -6;  break;
            case KeyEvent.KEYCODE_BUTTON_START  : btn = -7;  break;
            case KeyEvent.KEYCODE_BUTTON_THUMBL : btn = -8;  break;
            case KeyEvent.KEYCODE_BUTTON_THUMBR : btn = -9;  break;
            case KeyEvent.KEYCODE_BUTTON_L2     : btn = -10; break;
            case KeyEvent.KEYCODE_BUTTON_R2     : btn = -11; break;
            case KeyEvent.KEYCODE_BACK          : btn = KeyEvent.KEYCODE_ESCAPE; break;
            default                             : btn = keyCode;
        }

        boolean isDown = event.getAction() == KeyEvent.ACTION_DOWN;
        wrapper.onTouch(0, isDown ? -2 : -1, btn, 0);
        return true;
    }

    static {
        System.loadLibrary("game");
//      System.loadLibrary("gvr");
//        System.load("/storage/emulated/0/libMGD.so");
    }
}

// @TODO: use native OpenSL ES
class Sound {
    private short buffer[];
    private static AudioTrack audioTrack;

    void start(final Wrapper wrapper) {
        int rate = 44100;
        int size = AudioTrack.getMinBufferSize(rate, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT);
        //System.out.println(String.format("sound buffer size: %d", bufSize));
        buffer = new short[size / 2];

        try {
            audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, 44100, AudioFormat.CHANNEL_OUT_STEREO,
                    AudioFormat.ENCODING_PCM_16BIT, size, AudioTrack.MODE_STREAM);
        }catch (IllegalArgumentException e){
            System.out.println("Error: buffer size is zero");
            return;
        }

        try {
            audioTrack.play();
        }catch (NullPointerException e){
            System.out.println("Error: audioTrack null pointer on start()");
            return;
        }

        new Thread( new Runnable() {
            public void run() {
                while ( audioTrack.getPlayState() != AudioTrack.PLAYSTATE_STOPPED ) {
                    if (audioTrack.getPlayState() == AudioTrack.PLAYSTATE_PLAYING && wrapper.ready) {
                        synchronized (wrapper) {
                            Wrapper.nativeSoundFill(buffer);
                        }
                        audioTrack.write(buffer, 0, buffer.length);
                        audioTrack.flush();
                    } else
                        try {
                            Thread.sleep(10);
                        } catch(Exception e) {
                            //
                        }
                }
            }
        } ).start();
    }

    void stop() {
        try {
            audioTrack.flush();
            audioTrack.stop();
            audioTrack.release();
        }catch (NullPointerException e){
            System.out.println("Error: audioTrack null pointer on stop()");
        }
    }

    void play() {
        try {
            audioTrack.play();
        }catch (NullPointerException e){
            System.out.println("Error: audioTrack null pointer on play()");
        }
    }

    void pause() {
        try {
            audioTrack.pause();
        }catch (NullPointerException e){
            System.out.println("Error: audioTrack null pointer on pause()");
        };
    }
}

class Touch {
    int id, state;
    float x, y;
    Touch(int _id, int _state, float _x, float _y) {
        id = _id;
        state = _state;
        x = _x;
        y = _y;
    }
}

class Wrapper implements Renderer {
    public static native void nativeInit(String packName, String cacheDir, int levelOffset, int musicOffset);
    public static native void nativeFree();
    public static native void nativeReset();
    public static native void nativeResize(int w, int h);
    public static native void nativeUpdate();
    public static native void nativeRender();
    public static native void nativeTouch(int id, int state, float x, float y);
    public static native void nativeSoundFill(short buffer[]);

    Boolean ready = false;
    private String packName;
    private String cacheDir;
    private int levelOffset;
    private int musicOffset;
    private ArrayList<Touch> touch = new ArrayList<>();
    private Sound sound;

    void onCreate(String packName, String cacheDir, int levelOffset, int musicOffset) {
        this.packName = packName;
        this.cacheDir = cacheDir;
        this.levelOffset = levelOffset;
        this.musicOffset = musicOffset;

        sound = new Sound();
        sound.start(this);
    }

    void onDestroy() {
        sound.stop();
        nativeFree();
    }

    void onPause() {
        sound.pause();
    }

    void onResume() {
        sound.play();
        if (ready) nativeReset();
    }

    void onTouch(int id, int state, float x, float y) {
        synchronized (this) {
            touch.add(new Touch(id, state, x, y));
        }
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        synchronized (this) {
            for (int i = 0; i < touch.size(); i++) {
                Touch t = touch.get(i);
                nativeTouch(t.id, t.state, t.x, t.y);
            }
            touch.clear();
            nativeUpdate();
        }
        nativeRender();
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        nativeResize(width, height);
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        if (!ready) {
            nativeInit(packName, cacheDir, levelOffset, musicOffset);
            sound.play();
            ready = true;
        }
    }
}
