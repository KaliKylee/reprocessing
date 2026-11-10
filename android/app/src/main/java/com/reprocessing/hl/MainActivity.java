package com.reprocessing.hl;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.ColorStateList;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.RippleDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowInsets;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends Activity {

    private static final String GAMEDIR = "reprocessing";
    private static final String PREFS_NAME = "reprocessing_settings";
    private static final String PREF_GAME_ARGS = "game_arguments";
    private static final String DEFAULT_GAME_ARGS = "-console -log";

    private EditText gameArgsInput;

    private static final String MOD_VERSION = "1.4";

    private static final String URL_GITHUB = "https://github.com/KaliKylee/hlsdk-portable/tree/half-force";
    private static final String URL_YOUTUBE = "https://youtube.com/@kalikyle3";
    private static final String URL_DISCORD = "https://discord.gg/fUnXU4RZgn";
    private static final String URL_ENGINE = "https://github.com/FWGS/xash3d-fwgs/releases/tag/continuous";

    private static final String[][] WALLETS = {
        {"USDT  \u2022  Polygon", "0xe3c825c7eF4a3609E66b5D92C671d5C23dE78368"},
        {"USDT  \u2022  Ethereum", "0xe3c825c7eF4a3609E66b5D92C671d5C23dE78368"},
        {"USDT  \u2022  Solana", "G8br5Hgu9jSV1d84g7LfeNMBDJYRwwhntBBebA52QdLk"},
    };

    private static final int BG = 0xFF1A0508;
    private static final int PANEL = 0xFF2B0B10;
    private static final int RED = 0xFFDC143C;
    private static final int RED_DARK = 0xFF7A0C1E;
    private static final int GREEN = 0xFFDC143C;
    private static final int GREEN_DEEP = 0xFF5C0A16;

    private static final String ACTION_MOD = "su.xash.engine.MOD";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Intent in = getIntent();

        if (in != null && ACTION_MOD.equals(in.getAction())) {
            launchGame();
            return;
        }

        if (Build.VERSION.SDK_INT >= 21) {
            Window w = getWindow();
            w.setStatusBarColor(BG);
            w.setNavigationBarColor(BG);
        }

        setContentView(buildUi());
    }

    private void launchGame() {
        String pkg = "su.xash.engine.test";

        try {
            getPackageManager().getPackageInfo(pkg, 0);
        } catch (PackageManager.NameNotFoundException e) {
            try {
                pkg = "su.xash.engine";
                getPackageManager().getPackageInfo(pkg, 0);
            } catch (PackageManager.NameNotFoundException ex) {
                Toast.makeText(this, "Instale o Xash3D FWGS para jogar", Toast.LENGTH_LONG).show();
                openUrl(URL_ENGINE);
                return;
            }
        }

        Intent game = new Intent();
        game.setComponent(new ComponentName(pkg, "su.xash.engine.XashActivity"));
        game.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);

        game.putExtra("gamedir", GAMEDIR);
        game.putExtra("gamelibdir", getApplicationInfo().nativeLibraryDir);
        game.putExtra("package", getPackageName());

        String gameArgs;

        if (gameArgsInput != null) {
            gameArgs = gameArgsInput.getText().toString().trim();

            getSharedPreferences(PREFS_NAME, MODE_PRIVATE)
                    .edit()
                    .putString(PREF_GAME_ARGS, gameArgs)
                    .apply();

        } else {
            gameArgs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE)
                    .getString(PREF_GAME_ARGS, DEFAULT_GAME_ARGS);
        }

        if (!gameArgs.isEmpty()) {
            game.putExtra("argv", gameArgs);
        }

        try {
            startActivity(game);
            finish();
        } catch (ActivityNotFoundException e) {
            Toast.makeText(this, "Nao foi possivel abrir o Xash3D FWGS", Toast.LENGTH_LONG).show();
        }
    }

    private void openUrl(String url) {
        try {
            startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url)));
        } catch (ActivityNotFoundException e) {
            Toast.makeText(this, "Nenhum app para abrir o link", Toast.LENGTH_SHORT).show();
        }
    }

    private View buildUi() {
        FrameLayout root = new FrameLayout(this);
        root.setBackgroundColor(BG);

        // evita que o campo de argumentos pegue o foco e abra o teclado sozinho
        root.setFocusableInTouchMode(true);
        root.requestFocus();

        root.setOnApplyWindowInsetsListener(new View.OnApplyWindowInsetsListener() {
            @Override
            public WindowInsets onApplyWindowInsets(View v, WindowInsets insets) {
                v.setPadding(
                        insets.getSystemWindowInsetLeft(),
                        insets.getSystemWindowInsetTop(),
                        insets.getSystemWindowInsetRight(),
                        insets.getSystemWindowInsetBottom()
                );
                return insets;
            }
        });

        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        scroll.setVerticalScrollBarEnabled(false);

        root.addView(scroll, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
        ));

        FrameLayout holder = new FrameLayout(this);

        scroll.addView(holder, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
        ));

        LinearLayout col = new LinearLayout(this);
        col.setOrientation(LinearLayout.VERTICAL);
        col.setGravity(Gravity.CENTER_HORIZONTAL);
        col.setPadding(dp(20), dp(24), dp(20), dp(24));

        int width = getResources().getDisplayMetrics().widthPixels - dp(16);

        if (width > dp(460)) {
            width = dp(460);
        }

        FrameLayout.LayoutParams colLp = new FrameLayout.LayoutParams(
                width,
                ViewGroup.LayoutParams.WRAP_CONTENT,
                Gravity.CENTER
        );

        holder.addView(col, colLp);

        TextView title = text("REPROCESSING", 32, RED, true);
        title.setLetterSpacing(0.12f);
        title.setGravity(Gravity.CENTER);
        col.addView(title, wrap());

        TextView version = text("VERS\u00c3O " + MOD_VERSION, 15, GREEN, true);
        version.setLetterSpacing(0.3f);
        version.setGravity(Gravity.CENTER);

        LinearLayout.LayoutParams vLp = wrap();
        vLp.topMargin = dp(2);
        col.addView(version, vLp);

        View line = new View(this);
        line.setBackgroundColor(GREEN_DEEP);

        LinearLayout.LayoutParams lineLp = new LinearLayout.LayoutParams(dp(120), dp(2));
        lineLp.topMargin = dp(18);
        lineLp.bottomMargin = dp(26);
        col.addView(line, lineLp);

        TextView play = text("PLAY", 24, RED, true);
        play.setLetterSpacing(0.25f);
        play.setGravity(Gravity.CENTER);
        play.setBackground(ripple(box(RED_DARK, RED, 2, 14)));

        play.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                launchGame();
            }
        });

        LinearLayout.LayoutParams playLp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                dp(64)
        );

        col.addView(play, playLp);

        // ARGUMENTOS DE INICIALIZACAO

        TextView argsLabel = text("ARGUMENTOS DE INICIALIZA\u00c7\u00c3O", 12, GREEN, true);
        argsLabel.setLetterSpacing(0.08f);

        LinearLayout.LayoutParams argsLabelLp = wrap();
        argsLabelLp.topMargin = dp(16);
        col.addView(argsLabel, argsLabelLp);

        gameArgsInput = new EditText(this);
        gameArgsInput.setSingleLine(true);
        gameArgsInput.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        gameArgsInput.setTextColor(0xFFE8E8E8);
        gameArgsInput.setHintTextColor(0xFF858585);
        gameArgsInput.setHint("-console -log");

        gameArgsInput.setText(
                getSharedPreferences(PREFS_NAME, MODE_PRIVATE)
                        .getString(PREF_GAME_ARGS, DEFAULT_GAME_ARGS)
        );

        gameArgsInput.setPadding(dp(14), dp(12), dp(14), dp(12));
        gameArgsInput.setBackground(box(PANEL, GREEN_DEEP, 1, 10));

        LinearLayout.LayoutParams argsInputLp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                dp(50)
        );

        argsInputLp.topMargin = dp(8);
        col.addView(gameArgsInput, argsInputLp);

        TextView argsHint = text(
                "Personalize os par\u00e2metros enviados ao Xash3D FWGS.",
                11, GREEN, false
        );

        LinearLayout.LayoutParams argsHintLp = wrap();
        argsHintLp.topMargin = dp(5);
        col.addView(argsHint, argsHintLp);

        // LINKS

        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);

        LinearLayout.LayoutParams rowLp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
        );

        rowLp.topMargin = dp(18);
        col.addView(row, rowLp);

        row.addView(
                linkButton(new PolyIcon(GITHUB, GREEN, dp(30)), "GitHub", URL_GITHUB),
                cell(false)
        );

        row.addView(
                linkButton(new YouTubeIcon(GREEN, dp(30)), "YouTube", URL_YOUTUBE),
                cell(true)
        );

        row.addView(
                linkButton(new PolyIcon(DISCORD, GREEN, dp(30)), "Discord", URL_DISCORD),
                cell(true)
        );

        // APOIE-ME

        TextView support = text("APOIE-ME", 16, GREEN, true);
        support.setLetterSpacing(0.2f);
        support.setGravity(Gravity.CENTER);
        support.setBackground(ripple(box(PANEL, GREEN_DEEP, 2, 14)));

        support.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                showSupportDialog();
            }
        });

        LinearLayout.LayoutParams supLp = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                dp(52)
        );

        supLp.topMargin = dp(12);
        col.addView(support, supLp);

        TextView credit = text("Port by Kali Kyle", 14, RED, false);
        credit.setGravity(Gravity.CENTER);

        LinearLayout.LayoutParams crLp = wrap();
        crLp.topMargin = dp(30);
        col.addView(credit, crLp);

        TextView hint = text("Requer o app Xash3D FWGS instalado", 12, GREEN, false);
        hint.setGravity(Gravity.CENTER);

        LinearLayout.LayoutParams hintLp = wrap();
        hintLp.topMargin = dp(4);
        col.addView(hint, hintLp);

        return root;
    }

    private View linkButton(Drawable icon, String label, final String url) {
        LinearLayout b = new LinearLayout(this);
        b.setOrientation(LinearLayout.VERTICAL);
        b.setGravity(Gravity.CENTER);
        b.setPadding(dp(4), dp(12), dp(4), dp(10));
        b.setBackground(ripple(box(PANEL, GREEN_DEEP, 2, 14)));

        ImageView iv = new ImageView(this);
        iv.setImageDrawable(icon);

        b.addView(iv, new LinearLayout.LayoutParams(dp(30), dp(30)));

        TextView t = text(label, 12, GREEN, true);
        t.setGravity(Gravity.CENTER);

        LinearLayout.LayoutParams tLp = wrap();
        tLp.topMargin = dp(6);
        b.addView(t, tLp);

        b.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                openUrl(url);
            }
        });

        return b;
    }

    private LinearLayout.LayoutParams cell(boolean leftMargin) {
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                0,
                ViewGroup.LayoutParams.WRAP_CONTENT,
                1f
        );

        if (leftMargin) {
            lp.leftMargin = dp(10);
        }

        return lp;
    }

    // JANELA APOIE-ME

    private void showSupportDialog() {
        LinearLayout box = new LinearLayout(this);
        box.setOrientation(LinearLayout.VERTICAL);
        box.setPadding(dp(20), dp(20), dp(20), dp(16));

        TextView title = text("APOIE O PROJETO", 18, RED, true);
        title.setLetterSpacing(0.15f);
        box.addView(title, wrap());

        TextView note = text(
                "Envie somente USDT pela rede indicada e confira o endere\u00e7o antes de enviar. Obrigado por apoiar!",
                13, GREEN, false
        );

        LinearLayout.LayoutParams noteLp = wrap();
        noteLp.topMargin = dp(8);
        noteLp.bottomMargin = dp(8);
        box.addView(note, noteLp);

        for (int i = 0; i < WALLETS.length; i++) {
            final String label = WALLETS[i][0];
            final String address = WALLETS[i][1];

            LinearLayout card = new LinearLayout(this);
            card.setOrientation(LinearLayout.VERTICAL);
            card.setPadding(dp(14), dp(12), dp(14), dp(12));
            card.setBackground(box(PANEL, GREEN_DEEP, 2, 12));

            card.addView(text(label, 14, RED, true), wrap());

            TextView addr = text(address, 12, GREEN, false);
            addr.setTypeface(Typeface.MONOSPACE);
            addr.setTextIsSelectable(true);

            LinearLayout.LayoutParams addrLp = wrap();
            addrLp.topMargin = dp(6);
            card.addView(addr, addrLp);

            TextView copy = text("COPIAR", 13, RED, true);
            copy.setLetterSpacing(0.2f);
            copy.setGravity(Gravity.CENTER);
            copy.setBackground(ripple(box(0x00000000, RED, 2, 10)));

            copy.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    copyToClipboard(label, address);
                }
            });

            LinearLayout.LayoutParams copyLp = new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    dp(40)
            );

            copyLp.topMargin = dp(10);
            card.addView(copy, copyLp);

            LinearLayout.LayoutParams cardLp = new LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT
            );

            cardLp.topMargin = dp(8);
            box.addView(card, cardLp);
        }

        ScrollView sv = new ScrollView(this);

        sv.addView(box, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
        ));

        AlertDialog dialog = new AlertDialog.Builder(this)
                .setView(sv)
                .setPositiveButton("Fechar", null)
                .create();

        dialog.getWindow().setBackgroundDrawable(box(BG, GREEN_DEEP, 2, 18));
        dialog.show();
        dialog.getButton(AlertDialog.BUTTON_POSITIVE).setTextColor(GREEN);
    }

    private void copyToClipboard(String label, String address) {
        ClipboardManager cm = (ClipboardManager)
                getSystemService(Context.CLIPBOARD_SERVICE);

        if (cm != null) {
            cm.setPrimaryClip(ClipData.newPlainText(label, address));
            Toast.makeText(this, "Endere\u00e7o copiado", Toast.LENGTH_SHORT).show();
        }
    }

    // UTILIDADES

    private int dp(int v) {
        return Math.round(TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP,
                v,
                getResources().getDisplayMetrics()
        ));
    }

    private LinearLayout.LayoutParams wrap() {
        return new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
        );
    }

    private TextView text(String s, int sp, int color, boolean bold) {
        TextView t = new TextView(this);
        t.setText(s);
        t.setTextSize(TypedValue.COMPLEX_UNIT_SP, sp);
        t.setTextColor(color);
        t.setTypeface(bold ? Typeface.DEFAULT_BOLD : Typeface.DEFAULT);
        return t;
    }

    private GradientDrawable box(int fill, int stroke, int strokeDp, int radiusDp) {
        GradientDrawable g = new GradientDrawable();
        g.setShape(GradientDrawable.RECTANGLE);
        g.setColor(fill);
        g.setStroke(dp(strokeDp), stroke);
        g.setCornerRadius(dp(radiusDp));
        return g;
    }

    private Drawable ripple(Drawable content) {
        return new RippleDrawable(
                ColorStateList.valueOf(0x33FFFFFF),
                content,
                null
        );
    }

    // ICONES

    private static final float[][] DISCORD = {
        {20.32f, 4.37f, 19.73f, 4.11f, 19.14f, 3.87f, 18.53f, 3.65f, 17.92f, 3.45f, 17.31f, 3.27f, 16.69f, 3.11f, 16.06f, 2.97f, 15.43f, 2.85f, 15.41f, 2.85f, 15.39f, 2.86f, 15.36f, 2.88f, 15.22f, 3.13f, 15.10f, 3.38f, 14.97f, 3.64f, 14.85f, 3.90f, 14.75f, 4.14f, 13.64f, 4.01f, 12.54f, 3.94f, 11.44f, 3.94f, 10.35f, 4.01f, 9.26f, 4.14f, 9.15f, 3.90f, 9.03f, 3.64f, 8.90f, 3.38f, 8.77f, 3.13f, 8.64f, 2.89f, 8.62f, 2.87f, 8.60f, 2.86f, 8.57f, 2.85f, 7.93f, 2.97f, 7.31f, 3.11f, 6.69f, 3.27f, 6.07f, 3.45f, 5.46f, 3.65f, 4.86f, 3.87f, 4.26f, 4.11f, 3.68f, 4.37f, 3.65f, 4.38f, 2.04f, 7.17f, 0.93f, 9.92f, 0.27f, 12.65f, 0.01f, 15.36f, 0.10f, 18.06f, 0.11f, 18.08f, 0.12f, 18.10f, 1.35f, 18.95f, 2.56f, 19.66f, 3.76f, 20.25f, 4.95f, 20.74f, 6.12f, 21.14f, 6.15f, 21.15f, 6.17f, 21.14f, 6.19f, 21.13f, 6.48f, 20.73f, 6.74f, 20.34f, 6.98f, 19.94f, 7.21f, 19.54f, 7.43f, 19.12f, 7.44f, 19.09f, 7.44f, 19.06f, 7.42f, 19.03f, 7.39f, 19.02f, 7.00f, 18.86f, 6.62f, 18.69f, 6.25f, 18.51f, 5.88f, 18.32f, 5.52f, 18.12f, 5.49f, 18.10f, 5.48f, 18.06f, 5.49f, 18.02f, 5.51f, 18.00f, 5.59f, 17.94f, 5.66f, 17.88f, 5.74f, 17.82f, 5.81f, 17.76f, 5.88f, 17.70f, 5.91f, 17.69f, 5.93f, 17.69f, 5.95f, 17.69f, 8.35f, 18.55f, 10.79f, 18.99f, 13.23f, 18.99f, 15.66f, 18.55f, 18.02f, 17.69f, 18.05f, 17.69f, 18.07f, 17.69f, 18.10f, 17.70f, 18.17f, 17.76f, 18.25f, 17.82f, 18.32f, 17.88f, 18.40f, 17.94f, 18.47f, 18.00f, 18.50f, 18.02f, 18.50f, 18.06f, 18.49f, 18.10f, 18.47f, 18.12f, 18.24f, 18.25f, 18.01f, 18.38f, 17.78f, 18.49f, 17.55f, 18.61f, 17.31f, 18.72f, 17.08f, 18.82f, 16.84f, 18.92f, 16.59f, 19.01f, 16.57f, 19.03f, 16.55f, 19.06f, 16.55f, 19.09f, 16.55f, 19.12f, 16.78f, 19.54f, 17.01f, 19.94f, 17.26f, 20.34f, 17.51f, 20.73f, 17.78f, 21.11f, 17.81f, 21.14f, 17.83f, 21.15f, 17.85f, 21.15f, 19.04f, 20.74f, 20.23f, 20.25f, 21.43f, 19.66f, 22.64f, 18.95f, 23.87f, 18.11f, 23.89f, 18.09f, 23.90f, 18.07f, 23.98f, 15.03f, 23.64f, 12.16f, 22.91f, 9.44f, 21.81f, 6.85f, 20.35f, 4.40f, 20.33f, 4.38f},
        {8.02f, 15.33f, 7.34f, 15.21f, 6.75f, 14.86f, 6.28f, 14.34f, 5.97f, 13.68f, 5.86f, 12.91f, 5.97f, 12.15f, 6.28f, 11.48f, 6.74f, 10.96f, 7.33f, 10.62f, 8.02f, 10.49f, 8.71f, 10.62f, 9.31f, 10.96f, 9.77f, 11.49f, 10.08f, 12.15f, 10.18f, 12.91f, 10.07f, 13.68f, 9.76f, 14.34f, 9.30f, 14.86f, 8.71f, 15.21f, 8.02f, 15.33f},
        {15.99f, 15.33f, 15.32f, 15.21f, 14.72f, 14.86f, 14.26f, 14.34f, 13.95f, 13.68f, 13.84f, 12.91f, 13.95f, 12.15f, 14.25f, 11.48f, 14.72f, 10.96f, 15.31f, 10.62f, 15.99f, 10.49f, 16.69f, 10.62f, 17.28f, 10.96f, 17.75f, 11.49f, 18.05f, 12.15f, 18.15f, 12.91f, 18.04f, 13.68f, 17.74f, 14.34f, 17.28f, 14.86f, 16.68f, 15.21f, 15.99f, 15.33f}
    };

    private static final float[][] GITHUB = {
        {12.00f, 0.30f, 8.21f, 0.91f, 4.91f, 2.61f, 2.31f, 5.21f, 0.61f, 8.50f, 0.00f, 12.30f, 0.40f, 15.37f, 1.52f, 18.14f, 3.26f, 20.52f, 5.52f, 22.40f, 8.21f, 23.68f, 8.52f, 23.70f, 8.75f, 23.62f, 8.91f, 23.48f, 9.00f, 23.30f, 9.03f, 23.11f, 9.02f, 22.88f, 9.02f, 22.55f, 9.02f, 22.13f, 9.01f, 21.63f, 9.01f, 21.07f, 7.31f, 21.18f, 6.15f, 20.81f, 5.43f, 20.23f, 5.07f, 19.69f, 4.97f, 19.46f, 4.62f, 18.74f, 4.26f, 18.24f, 3.95f, 17.92f, 3.72f, 17.75f, 3.63f, 17.70f, 3.22f, 17.34f, 3.19f, 17.12f, 3.37f, 17.01f, 3.60f, 16.98f, 3.72f, 16.97f, 4.37f, 17.13f, 4.88f, 17.44f, 5.26f, 17.80f, 5.48f, 18.09f, 5.55f, 18.21f, 6.26f, 19.04f, 7.05f, 19.44f, 7.83f, 19.51f, 8.53f, 19.40f, 9.05f, 19.21f, 9.14f, 18.77f, 9.27f, 18.39f, 9.43f, 18.07f, 9.61f, 17.81f, 9.81f, 17.60f, 8.22f, 17.31f, 6.73f, 16.71f, 5.50f, 15.66f, 4.66f, 14.02f, 4.34f, 11.67f, 4.40f, 10.91f, 4.56f, 10.21f, 4.81f, 9.57f, 5.15f, 8.98f, 5.58f, 8.45f, 5.48f, 8.16f, 5.37f, 7.68f, 5.33f, 7.02f, 5.41f, 6.21f, 5.68f, 5.27f, 5.81f, 5.25f, 6.18f, 5.26f, 6.83f, 5.40f, 7.76f, 5.78f, 8.98f, 6.50f, 9.57f, 6.36f, 10.16f, 6.25f, 10.76f, 6.17f, 11.37f, 6.12f, 11.98f, 6.10f, 12.60f, 6.12f, 13.20f, 6.17f, 13.81f, 6.25f, 14.40f, 6.36f, 14.98f, 6.50f, 16.20f, 5.78f, 17.13f, 5.40f, 17.77f, 5.26f, 18.15f, 5.25f, 18.27f, 5.27f, 18.54f, 6.21f, 18.62f, 7.02f, 18.58f, 7.68f, 18.48f, 8.16f, 18.39f, 8.45f, 18.81f, 8.98f, 19.15f, 9.57f, 19.41f, 10.21f, 19.56f, 10.91f, 19.62f, 11.67f, 19.31f, 14.03f, 18.46f, 15.66f, 17.22f, 16.71f, 15.74f, 17.30f, 14.14f, 17.59f, 14.39f, 17.85f, 14.61f, 18.20f, 14.79f, 18.65f, 14.91f, 19.18f, 14.95f, 19.81f, 14.95f, 20.73f, 14.95f, 21.55f, 14.94f, 22.23f, 14.94f, 22.76f, 14.94f, 23.10f, 14.97f, 23.29f, 15.05f, 23.47f, 15.21f, 23.61f, 15.44f, 23.68f, 15.76f, 23.67f, 18.46f, 22.39f, 20.74f, 20.51f, 22.48f, 18.13f, 23.60f, 15.36f, 24.00f, 12.30f, 23.39f, 8.50f, 21.68f, 5.21f, 19.09f, 2.61f, 15.79f, 0.91f, 12.00f, 0.30f}
    };

    private static class PolyIcon extends Drawable {
        private final float[][] polys;
        private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final int size;
        private final Path path = new Path();

        PolyIcon(float[][] polys, int color, int size) {
            this.polys = polys;
            this.size = size;
            paint.setColor(color);
            paint.setStyle(Paint.Style.FILL);
            path.setFillType(Path.FillType.EVEN_ODD);
        }

        @Override
        public void draw(Canvas canvas) {
            Rect b = getBounds();
            float s = Math.min(b.width(), b.height()) / 24f;
            float ox = b.left + (b.width() - 24f * s) / 2f;
            float oy = b.top + (b.height() - 24f * s) / 2f;

            path.reset();

            for (int i = 0; i < polys.length; i++) {
                float[] p = polys[i];
                path.moveTo(ox + p[0] * s, oy + p[1] * s);

                for (int k = 2; k + 1 < p.length; k += 2) {
                    path.lineTo(ox + p[k] * s, oy + p[k + 1] * s);
                }

                path.close();
            }

            canvas.drawPath(path, paint);
        }

        @Override public int getIntrinsicWidth() { return size; }
        @Override public int getIntrinsicHeight() { return size; }
        @Override public void setAlpha(int alpha) { paint.setAlpha(alpha); }
        @Override public void setColorFilter(ColorFilter cf) { paint.setColorFilter(cf); }
        @Override public int getOpacity() { return PixelFormat.TRANSLUCENT; }
    }

    private static class YouTubeIcon extends Drawable {
        private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final int size;
        private final Path path = new Path();

        YouTubeIcon(int color, int size) {
            this.size = size;
            paint.setColor(color);
            paint.setStyle(Paint.Style.FILL);
            path.setFillType(Path.FillType.EVEN_ODD);
        }

        @Override
        public void draw(Canvas canvas) {
            Rect b = getBounds();
            float s = Math.min(b.width(), b.height()) / 24f;
            float ox = b.left + (b.width() - 24f * s) / 2f;
            float oy = b.top + (b.height() - 24f * s) / 2f;

            path.reset();

            path.addRoundRect(
                    ox + 1f * s, oy + 4.5f * s,
                    ox + 23f * s, oy + 19.5f * s,
                    4.5f * s, 4.5f * s,
                    Path.Direction.CW
            );

            path.moveTo(ox + 9.75f * s, oy + 8.6f * s);
            path.lineTo(ox + 15.75f * s, oy + 12f * s);
            path.lineTo(ox + 9.75f * s, oy + 15.4f * s);
            path.close();

            canvas.drawPath(path, paint);
        }

        @Override public int getIntrinsicWidth() { return size; }
        @Override public int getIntrinsicHeight() { return size; }
        @Override public void setAlpha(int alpha) { paint.setAlpha(alpha); }
        @Override public void setColorFilter(ColorFilter cf) { paint.setColorFilter(cf); }
        @Override public int getOpacity() { return PixelFormat.TRANSLUCENT; }
    }
}
