package org.reigncarnated.assetprobe;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
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
        title.setText("Reigncarnated — Native asset test");
        title.setTextSize(23);
        panel.addView(title);
        TextView help = new TextView(this);
        help.setText("Choose base.zwp from your Dark Reign 2 installation. This tests native ARM64 archive access. It does not run the game.\n\nYour selected file is copied into app-private temporary storage for testing. No network access is used.");
        help.setTextSize(16);
        panel.addView(help);
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
            send.putExtra(Intent.EXTRA_TEXT, report);
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
                answer = "Reigncarnated asset probe v0.2\nDevice: " + android.os.Build.MANUFACTURER + " " + android.os.Build.MODEL
                    + "\nAndroid API: " + android.os.Build.VERSION.SDK_INT + "\n" + inspectNative(local.getAbsolutePath());
            } catch (Exception e) {
                answer = "TEST FAILED\n" + e.getClass().getSimpleName() + ": " + e.getMessage();
            } finally {
                if (local != null) local.delete();
            }
            final String completed = answer;
            runOnUiThread(() -> {
                if (isFinishing() || isDestroyed()) return;
                report = completed;
                result.setText(report);
                choose.setEnabled(true);
            });
        }, "ReignAssetProbe").start();
    }
}
