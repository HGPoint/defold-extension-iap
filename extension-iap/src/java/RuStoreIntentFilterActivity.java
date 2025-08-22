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
		Class<?> activityClass = null;
		try {
			activityClass = Class.forName(activityClassName);
		} catch(ClassNotFoundException ex) {
		}

		return activityClass;
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		Log.d(TAG, "INFO:RUSTOREPAY: onCreate()");

		if (savedInstanceState == null) {
			RuStorePay.INSTANCE.proceedIntent(getIntent());
		}

		startGameActivity(null, defoldActivityClassName);
		finish();
	}

	@Override
	public void onNewIntent(Intent newIntent)
	{
		Log.d(TAG, "INFO:RUSTOREPAY: onNewIntent()");

		super.onNewIntent(newIntent);

		RuStorePay.INSTANCE.proceedIntent(newIntent);
		startGameActivity(newIntent, defoldActivityClassName);
		finish();
	}

	private void startGameActivity(Intent intent, String gameActivityClassName) {
		Log.d(TAG, "INFO:RUSTOREPAY: startGameActivity()");
		
		Class<?> gameActivityClass = getActivityClass(gameActivityClassName);
		if (gameActivityClass != null) {
			Intent newIntent = new Intent(this, gameActivityClass);
			//newIntent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
			Log.d(TAG, "INFO:RUSTOREPAY: Intent " + getIntent().toString());
			//Log.d(TAG, "INFO:RUSTOREPAY: newIntent: " + newIntent.toString());
			if (intent != null) newIntent.putExtras(intent.getExtras());
			startActivity(newIntent);
		}
	}

    public static String getUUID() {
        return UUID.randomUUID().toString();
    }
}
