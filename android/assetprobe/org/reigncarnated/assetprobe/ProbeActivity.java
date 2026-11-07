package org.reigncarnated.assetprobe;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.opengl.GLSurfaceView;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

public final class ProbeActivity extends Activity {
    static { System.loadLibrary("reign_asset_probe"); }
    private static final int PICK_PACK = 10;
    private TextView result;
    private Button choose;
    private Preview preview;
    private TextView rendererResult;
    private String rendererReport = "Renderer not initialized.";
    private native long createPreviewNative();
    private native String previewReportNative(long handle);
    private native String drawPreviewNative(long handle, int width, int height);
    private native void destroyPreviewNative(long handle, boolean contextLost);
    private native byte[] loadAtlasNative(String path);
    private native String uploadAtlasNative(long handle, byte[] rgba);

    private void rendererStatus(String message) {
        runOnUiThread(() -> {
            if (isFinishing() || isDestroyed()) return;
            rendererReport = message;
            rendererResult.setText(message);
        });
    }
    private final class Preview extends GLSurfaceView implements GLSurfaceView.Renderer {
        private long handle;
        private int width, height, generations;
        private String driverInfo = "";
        private volatile byte[] image;
        Preview() {
            super(ProbeActivity.this);
            setEGLContextClientVersion(3);
            setEGLConfigChooser(8,8,8,8,16,0);
            setRenderer(this);
            setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
        }
        @Override public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            if (handle != 0) destroyPreviewNative(handle,true);
            handle = 0; generations++;
            driverInfo = "Device: " + android.os.Build.MANUFACTURER + " " + android.os.Build.MODEL + "\nAPI: " + android.os.Build.VERSION.SDK_INT + "\nGL: " + gl.glGetString(GL10.GL_VERSION) + "\nGPU: " + gl.glGetString(GL10.GL_RENDERER);
        }
        @Override public void onSurfaceChanged(GL10 gl, int w, int h) {
            width=w; height=h;
            if (handle != 0) rendererStatus(driverInfo + "\nContext generation: " + generations + "\nSurface: " + width + " x " + height + "\n" + previewReportNative(handle));
        }
        @Override public void onDrawFrame(GL10 gl) {
            if (width < 32 || height < 32) return;
            if (handle == 0) {
                handle = createPreviewNative();
                rendererStatus(driverInfo + "\nContext generation: " + generations + "\nSurface: " + width + " x " + height + "\n" + previewReportNative(handle));
                if (handle != 0 && image != null) {
                    String error=uploadAtlasNative(handle,image);
                    if (!error.isEmpty()) rendererStatus("TEXTURE FAIL after context recreation: " + error);
                }
            }
            if (handle != 0) {
                String error=drawPreviewNative(handle,width,height);
                if (!error.isEmpty()) rendererStatus("RENDERER FAIL: " + error);
            }
        }
        void showAtlas(byte[] pixels) {
            image=pixels;
            queueEvent(() -> {
                if (handle != 0) {
                    String error=uploadAtlasNative(handle,pixels);
                    if (!error.isEmpty()) rendererStatus("TEXTURE FAIL: " + error);
                }
            });
            requestRender();
        }
        void pauseRenderer() {
            queueEvent(() -> { if (handle != 0) { destroyPreviewNative(handle,false); handle=0; } });
            super.onPause();
        }
    }
    private String report = "No archive tested yet.";
    private native String inspectNative(String path);

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        LinearLayout panel = new LinearLayout(this);
        panel.setOrientation(LinearLayout.VERTICAL);
        int pad = (int)(20 * getResources().getDisplayMetrics().density);
        panel.setPadding(pad, pad, pad, pad);
        panel.setOnApplyWindowInsetsListener((view, insets) -> {
            view.setPadding(pad + insets.getSystemWindowInsetLeft(), pad + insets.getSystemWindowInsetTop(),
                pad + insets.getSystemWindowInsetRight(), pad + insets.getSystemWindowInsetBottom());
            return insets;
        });
        TextView title = new TextView(this);
        title.setText("Reigncarnated — Renderer bring-up");
        title.setTextSize(23);
        panel.addView(title);
        TextView help = new TextView(this);
        help.setText("Choose base.zwp from your Dark Reign 2 installation. This checks native archive access and the GLES renderer, then previews the original cursor texture. It does not run the game.\n\nYour selected file is copied into app-private temporary storage for testing. No network access is used.");
        help.setTextSize(16);
        panel.addView(help);
        preview = new Preview();
        panel.addView(preview,new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,(int)(220*getResources().getDisplayMetrics().density)));
        rendererResult = new TextView(this);
        rendererResult.setTextSize(13);
        rendererResult.setText(rendererReport);
        panel.addView(rendererResult);
        choose = new Button(this);
        choose.setText("Choose base.zwp");
        choose.setOnClickListener(v -> {
            Intent picker = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            picker.setType("*/*");
            picker.addCategory(Intent.CATEGORY_OPENABLE);
            startActivityForResult(picker, PICK_PACK);
        });
        panel.addView(choose);
        Button share = new Button(this);
        share.setText("Share test report");
        share.setOnClickListener(v -> {
            Intent send = new Intent(Intent.ACTION_SEND);
            send.setType("text/plain");
            send.putExtra(Intent.EXTRA_TEXT, report + "\n\n" + rendererReport);
            startActivity(Intent.createChooser(send, "Share Reigncarnated report"));
        });
        panel.addView(share);
        result = new TextView(this);
        result.setTextSize(15);
        result.setTextIsSelectable(true);
        if (state != null) report = state.getString("report", report);
        result.setText(report);
        panel.addView(result);
        ScrollView scroll = new ScrollView(this);
        scroll.addView(panel);
        setContentView(scroll);
    }
    @Override protected void onResume() { super.onResume(); if (preview != null) preview.onResume(); }
    @Override protected void onPause() { if (preview != null) preview.pauseRenderer(); super.onPause(); }
    @Override public void onSaveInstanceState(Bundle state) {
        state.putString("report", report);
        super.onSaveInstanceState(state);
    }
    @Override protected void onActivityResult(int request, int code, Intent data) {
        super.onActivityResult(request, code, data);
        if (request != PICK_PACK || code != RESULT_OK || data == null || data.getData() == null) return;
        final Uri uri = data.getData();
        choose.setEnabled(false);
        result.setText("Copying and checking selected archive…");
        new Thread(() -> {
            String answer;
            byte[] atlas = null;
            File local = null;
            try {
                local = File.createTempFile("reign-pack-", ".zwp", getCacheDir());
                long copied = 0;
                try (InputStream in = getContentResolver().openInputStream(uri);
                     FileOutputStream out = new FileOutputStream(local)) {
                    if (in == null) throw new java.io.IOException("Cannot open selected file");
                    byte[] buffer = new byte[256 * 1024];
                    int n;
                    while ((n = in.read(buffer)) != -1) {
                        copied += n;
                        if (copied > 512L * 1024 * 1024) throw new java.io.IOException("Archive exceeds the 512 MiB diagnostic limit");
                        out.write(buffer, 0, n);
                    }
                }
                atlas = loadAtlasNative(local.getAbsolutePath());
                answer = "Reigncarnated renderer bring-up v0.4\nDevice: " + android.os.Build.MANUFACTURER + " " + android.os.Build.MODEL
                    + "\nAndroid API: " + android.os.Build.VERSION.SDK_INT + "\n" + inspectNative(local.getAbsolutePath()) + "\nCursor atlas decoded: " + (atlas == null ? "NO" : "YES");
            } catch (Exception e) {
                answer = "TEST FAILED\n" + e.getClass().getSimpleName() + ": " + e.getMessage();
            } finally {
                if (local != null) local.delete();
            }
            final String completed = answer;
            final byte[] completedAtlas = atlas;
            runOnUiThread(() -> {
                if (isFinishing() || isDestroyed()) return;
                report = completed;
                result.setText(report);
                choose.setEnabled(true);
                if (completedAtlas != null) preview.showAtlas(completedAtlas);
            });
        }, "ReignAssetProbe").start();
    }
}
