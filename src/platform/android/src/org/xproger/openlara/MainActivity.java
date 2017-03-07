package org.xproger.openlara;

import java.util.ArrayList;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.Renderer;
import android.os.Bundle;
import android.app.Activity;
import android.content.res.AssetFileDescriptor;
import android.util.Log;
import android.util.SparseIntArray;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnGenericMotionListener;
import android.view.View.OnKeyListener;
import android.view.View.OnTouchListener;
import android.view.Window;
import android.view.WindowManager;

public class MainActivity extends Activity implements OnTouchListener, OnGenericMotionListener, OnKeyListener {
    private GLSurfaceView view;
    private Wrapper wrapper;
    private SparseIntArray joys = new SparseIntArray();

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

        view = new GLSurfaceView(this);
        view.setEGLConfigChooser(8, 8, 8, 8, 16, 0);
        view.setEGLContextClientVersion(2);
        view.setRenderer(wrapper = new Wrapper());

        view.setFocusable(true);
        view.setFocusableInTouchMode(true);

        view.setOnTouchListener(this);
        view.setOnGenericMotionListener(this);
        view.setOnKeyListener(this);

        setContentView(view);
        try {
            String packName = getPackageManager().getPackageInfo(getPackageName(), 1).applicationInfo.sourceDir;
            AssetFileDescriptor fLevel = this.getResources().openRawResourceFd(R.raw.level2);
            AssetFileDescriptor fMusic = this.getResources().openRawResourceFd(R.raw.music);

            wrapper.onCreate(packName, (int)fLevel.getStartOffset(), (int)fMusic.getStartOffset());
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

    private int getJoyIndex(InputDevice dev) {
        int src = dev.getSources();
        if ((src & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD ||
                (src & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK) {

            int id = dev.getId();
            int index = joys.get(id, -1);

            if (index == -1) {
                index = joys.size();
                joys.append(id, index);
            }
            return index;
        }
        return -1;
    }

    @Override
    public boolean onGenericMotion(View v, MotionEvent event) {
        int index = getJoyIndex(event.getDevice());
        if (index == -1) return false;

        wrapper.onTouch(index, -3, event.getAxisValue(MotionEvent.AXIS_X),
                event.getAxisValue(MotionEvent.AXIS_Y));

        wrapper.onTouch(index, -4, event.getAxisValue(MotionEvent.AXIS_Z),
                event.getAxisValue(MotionEvent.AXIS_RZ));

        return true;
    }

    @Override
    public boolean onKey(View v, int keyCode, KeyEvent event) {
        int index = getJoyIndex(event.getDevice());
        if (index == -1) return false;

        int btn;

        switch (keyCode) {
            case KeyEvent.KEYCODE_BUTTON_A : btn = 0; break;
            case KeyEvent.KEYCODE_BUTTON_B : btn = 1; break;
            case KeyEvent.KEYCODE_BUTTON_X : btn = 2; break;
            case KeyEvent.KEYCODE_BUTTON_Y : btn = 3; break;
            default : btn = -1;
        }

        if (btn != -1) {
            wrapper.onTouch(index, event.getAction() == KeyEvent.ACTION_DOWN ? -2 : -1, btn, 0);
            return true;
        }
        return false;
    }

    static {
        System.loadLibrary("game");
    }
}

class Sound {
    private short buffer[];
    private static AudioTrack audioTrack;

    public void start(final Wrapper wrapper) {
        int bufferSize = AudioTrack.getMinBufferSize(22050, AudioFormat.CHANNEL_CONFIGURATION_STEREO, AudioFormat.ENCODING_PCM_16BIT);
        System.out.println(String.format("sound buffer size: %d", bufferSize));

        buffer = new short [bufferSize / 2];
        audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC,
                44100,
                AudioFormat.CHANNEL_CONFIGURATION_STEREO,
                AudioFormat.ENCODING_PCM_16BIT,
                bufferSize,
                AudioTrack.MODE_STREAM);
        audioTrack.play();

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
                            Thread.sleep(100);
                        } catch(Exception e) {
                            //
                        };
                }
            }
        } ).start();
    }

    public void stop() {
        audioTrack.flush();
        audioTrack.stop();
        audioTrack.release();
    }

    public void play() {
        audioTrack.play();
    }

    public void pause() {
        audioTrack.pause();
    }
}

class Touch {
    int id, state;
    float x, y;
    public Touch(int _id, int _state, float _x, float _y) {
        id = _id;
        state = _state;
        x = _x;
        y = _y;
    };
}

class Wrapper implements Renderer {
    public static native void nativeInit(String packName, int levelOffset, int musicOffset);
    public static native void nativeFree();
    public static native void nativeReset();
    public static native void nativeResize(int w, int h);
    public static native void nativeUpdate();
    public static native void nativeRender();
    public static native void nativeTouch(int id, int state, float x, float y);
    public static native void nativeSoundFill(short buffer[]);

    public Boolean ready = false;
    private String packName;
    private int levelOffset;
    private int musicOffset;
    private ArrayList<Touch> touch = new ArrayList<Touch>();
    private Sound sound;

    public void onCreate(String packName, int levelOffset, int musicOffset) {
        this.packName = packName;
        this.levelOffset = levelOffset;
        this.musicOffset = musicOffset;

        sound = new Sound();
        sound.start(this);
    }

    public void onDestroy() {
        sound.stop();
        nativeFree();
    }

    public void onPause() {
        sound.pause();
    }

    public void onResume() {
        sound.play();
        if (ready) nativeReset();
    }

    public void onTouch(int id, int state, float x, float y) {
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
            nativeInit(packName, levelOffset, musicOffset);
            sound.play();
            ready = true;
        }
    }
}
