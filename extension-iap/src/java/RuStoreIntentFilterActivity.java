package ru.rustore.defold.pay;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import ru.rustore.defold.pay.RuStorePay;
import java.util.UUID;
import android.util.Log;

public class RuStoreIntentFilterActivity extends Activity {

    public static final String TAG = "RuStoreIntentFilterActivity";
    
    private final String defoldActivityClassName = "com.dynamo.android.DefoldActivity";

    private Class<?> getActivityClass(String activityClassName) {
        try {
            return Class.forName(activityClassName);
        } catch(ClassNotFoundException ex) {
            return null;
        }
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

		Log.d(TAG, "INFO:RUSTOREPAY: onCreate()");
        
        if (savedInstanceState == null) {
            RuStorePay.INSTANCE.proceedIntent(getIntent());
        }

        if (!isTaskRoot()) {
            finish();
            return;
        }
        
        startGameActivity(defoldActivityClassName);
        finish();
    }
    
    @Override
    public void onNewIntent(Intent intent) {
		Log.d(TAG, "INFO:RUSTOREPAY: onNewIntent()");
        super.onNewIntent(intent);
        
        RuStorePay.INSTANCE.proceedIntent(intent);
    }
    
    private void startGameActivity(String gameActivityClassName) {
		Log.d(TAG, "INFO:RUSTOREPAY: startGameActivity()");
        Class<?> gameActivityClass = getActivityClass(gameActivityClassName);
        if (gameActivityClass != null) {
            Intent intent = new Intent(this, gameActivityClass);
            Log.d(TAG, "INFO:RUSTOREPAY: Intent " + getIntent().toString());

            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
            startActivity(intent);
        }
    }

    public static String getUUID() {
        return UUID.randomUUID().toString();
    }
}
