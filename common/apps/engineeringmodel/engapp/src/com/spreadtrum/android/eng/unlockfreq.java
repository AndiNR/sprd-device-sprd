package com.spreadtrum.android.eng;

import android.app.Activity;
import android.util.Log;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.os.Bundle;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import android.view.View;
import android.widget.Toast;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.AlertDialog.Builder;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.content.DialogInterface;

public class unlockfreq extends Activity {
	private static final String LOG_TAG = "engnetinfo";
	private int sockid = 0;
	private engfetch mEf;
	private String str=null;
	private EventHandler mHandler;
	public EditText  mET;
	private Button mButton;
	private Button mButton01;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.unlockfreq);

		initialpara();

		mET = (EditText)findViewById(R.id.editText1);
		mButton = (Button)findViewById(R.id.lock_button1);
		mButton01 = (Button)findViewById(R.id.clear_button1);
		clearEditText();
		mButton.setText("Unlock");
		mButton01.setText("Clear");

		mButton.setOnClickListener(new Button.OnClickListener(){
			public void onClick(View v) {
			// TODO Auto-generated method stub
			Log.d(LOG_TAG, "before engwrite");
           /*Add 20130201 Spreadst of 122488 crash when input a long number to UNLOCK FREQUENCY start */
            int len =mET.getText().toString().length();
            if(len>=10){
                AlertDialog.Builder builder = new AlertDialog.Builder(unlockfreq.this);
                builder.setTitle("number is too long").setMessage("the largest size of number is 9");
                builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                    }
                });
                Dialog alertDialog = builder.create();
                alertDialog.show();
                return;
            }
            /*Add 20130201 Spreadst of 122488 crash when input a long number to UNLOCK FREQUENCY end */
			Message m = mHandler.obtainMessage(engconstents.ENG_AT_SETSPFRQ, 0, 0, 0);
			mHandler.sendMessage(m);
			Log.d(LOG_TAG, "after engwrite");
			}
		});

		mButton01.setOnClickListener(new Button.OnClickListener(){
			public void onClick(View v) {
			// TODO Auto-generated method stub
		    	clearEditText();
			}
		
		});
    	}

	private void clearEditText() {
		// TODO Auto-generated method stub
		mET.setText("0");
	}

	private void initialpara() {
		// TODO Auto-generated method stub
		mEf = new engfetch();
		sockid = mEf.engopen();
		Looper looper;
		looper = Looper.myLooper();
		mHandler = new EventHandler(looper);
		mHandler.removeMessages(0);
	}

	private class EventHandler extends Handler{
		public EventHandler(Looper looper){
		    super(looper);
		}

		@Override
		public void handleMessage(Message msg){
			 switch(msg.what)
			 {
			    case engconstents.ENG_AT_SETSPFRQ:
					ByteArrayOutputStream outputBuffer = new ByteArrayOutputStream();
					DataOutputStream outputBufferStream = new DataOutputStream(outputBuffer);

					Log.e(LOG_TAG, "engopen sockid=" + sockid);
 /*Modify 20130205 Spreadst of 125480 change the method of creating cmd start*/
                    //str=String.format("%d,%d,%d,%d,%d", msg.what,3,1,Integer.parseInt(mET.getText().toString()),10088);
                    str=new StringBuilder().append(msg.what).append(",").append(3).append(",").append(1)
                        .append(",").append(Integer.parseInt(mET.getText().toString())).append(",")
                        .append(10088).toString();
 /*Modify 20130205 Spreadst of 125480 change the method of creating cmd end*/
					try {
					outputBufferStream.writeBytes(str);
					} catch (IOException e) {
					Log.e(LOG_TAG, "writebytes error");
					return;
					}
					mEf.engwrite(sockid,outputBuffer.toByteArray(),outputBuffer.toByteArray().length);

					int dataSize = 128;
					byte[] inputBytes = new byte[dataSize];
					int showlen= mEf.engread(sockid,inputBytes,dataSize);
					String str =new String(inputBytes,0,showlen); 

					if(str.equals("OK"))
					{
					Toast.makeText(getApplicationContext(), "Unlock Success.",Toast.LENGTH_SHORT).show(); 

					}
					else if(str.equals("ERROR"))
					{
					Toast.makeText(getApplicationContext(), "Unlock Failed.",Toast.LENGTH_SHORT).show(); 

					}
					else
					Toast.makeText(getApplicationContext(), "Unknown",Toast.LENGTH_SHORT).show(); 
				    break;
			 }
		}
	}


}

