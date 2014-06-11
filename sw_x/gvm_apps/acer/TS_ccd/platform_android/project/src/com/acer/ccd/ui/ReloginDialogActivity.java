package com.acer.ccd.ui;

import com.acer.ccd.R;
import com.acer.ccd.util.InternalDefines;
import com.acer.ccd.util.Utility;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

public class ReloginDialogActivity extends Activity {
    
    private static final String TAG = "ReloginDialogActivity"; 
    
    private TextView mTextEmail;
    private EditText mEditPassword;
    private Button mButtonLogin, mButtonCancel;
    
    private String mEmail;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.relogin_activity);
        findViews();
        setEvents();
        mEmail = Utility.getAccountId(ReloginDialogActivity.this);
        mTextEmail.setText(mEmail);
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        Log.i(TAG, "resultCode = " + resultCode);
        switch (resultCode) {
            case InternalDefines.PROGRESSING_RESULT_ACCOUNT_ERROR:
                Utility.showMessageDialog(this, R.string.signin_dialog_title_incorrect,
                        R.string.signin_dialog_content_incorrent);
                break;
            case InternalDefines.PROGRESSING_RESULT_SUCCESS:
                //addAccount();
                finish();
                break;
            default:
        }
    }
    
    private void findViews() {
        mTextEmail = (TextView) findViewById(R.id.text_email);
        mEditPassword = (EditText) findViewById(R.id.edit_password);
        mButtonLogin = (Button) findViewById(R.id.button_login);
        mButtonCancel = (Button) findViewById(R.id.button_cancel);
    }
    
    private void setEvents() {
        mButtonLogin.setOnClickListener(mLoginClick);
        mButtonCancel.setOnClickListener(mCancelClick);
    }
    
    private Button.OnClickListener mLoginClick = new Button.OnClickListener() {
        @Override
        public void onClick(View arg0) {
            Intent intent = new Intent(ReloginDialogActivity.this, ProgressingActivity.class);
            intent.putExtra(InternalDefines.BUNDLE_PROGRESS_TYPE, InternalDefines.PROGRESSING_TYPE_SIGNIN);
            intent.putExtra(InternalDefines.ACCOUNT_BUNDLE_EMAIL, mEmail);
            intent.putExtra(InternalDefines.ACCOUNT_BUNDLE_PASSWORD, mEditPassword.getText().toString());
            startActivityForResult(intent, 0);
        }
    };
    
    private Button.OnClickListener mCancelClick = new Button.OnClickListener() {
        @Override
        public void onClick(View arg0) {
            finish();
        }
    };
    
}
