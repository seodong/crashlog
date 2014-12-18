package com.example.crashlog;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

public class CrashlogExampleActivity extends Activity {
	private static final String CRASH_LOG_PATH = "/crashlog";
	private static final String APP_VERSION = "0.0.1";

	private static CrashlogExampleJni sJni = null;

	Button mButtonEnter;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_crashlog_example);

		mButtonEnter = (Button) findViewById(R.id.btn_enter);
		mButtonEnter.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				sJni.forceCrash();
			}
		});

		sJni = new CrashlogExampleJni();

		// String path = Environment.getExternalStorageDirectory().getPath() +
		// "/Android/data/"
		// + getApplicationContext().getPackageName() + CRASH_LOG_PATH;
		String path = Environment.getExternalStorageDirectory().getPath() + CRASH_LOG_PATH;
		sJni.setLogPath(path);
		Log.i("crashlog", path);
		sJni.setAppVersion(APP_VERSION);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.crashlog_example, menu);
		return true;
	}

}
