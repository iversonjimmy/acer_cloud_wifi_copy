/*
 *  Copyright 2011 Acer Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER INC.
 */

package com.acer.ccd.ui;

import com.acer.ccd.R;
import com.acer.ccd.util.InternalDefines;
import com.acer.ccd.util.Utility;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;

public class SignUpEntryActivity extends Activity {
    private EditText editFirstname;
    private EditText editLastname;
    private EditText editEmail;
    private EditText editConfirmEmail;
    private CheckBox checkboxRememberAccount;
    private EditText editPassword;

    private Button buttonBack;
    private Button buttonNext;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.sign_up_activity_entry);

        findViews();
        setEvents();
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode == InternalDefines.PROGRESSING_RESULT_UNKNOWN) {
            Utility.ToastMessage(this, R.string.message_unknown_error);
        } else if (resultCode == InternalDefines.PROGRESSING_RESULT_DUPLICATE) {
            Utility.ToastMessage(this, R.string.message_email_exists);
        } else if (resultCode == InternalDefines.PROGRESSING_RESULT_EMAIL_INVALID){
            Utility.ToastMessage(this, R.string.message_email_invalid);
        } else if (resultCode == InternalDefines.PROGRESSING_RESULT_ID_LENGTH) {
            Utility.ToastMessage(this, R.string.message_id_length_not_enough);
        } else if (resultCode == InternalDefines.PROGRESSING_RESULT_PW_LENGTH) {
            Utility.ToastMessage(this, R.string.message_pw_length_not_enough);
        }
    }

    private void setEvents() {
        buttonBack.setOnClickListener(buttonBackClickListener);
        buttonNext.setOnClickListener(buttonNextClickListener);
    }

    private void findViews() {
        editFirstname = (EditText) findViewById(R.id.signup_edit_firstname);
        editLastname = (EditText) findViewById(R.id.signup_edit_lastname);
        editEmail = (EditText) findViewById(R.id.signup_edit_email);
        editConfirmEmail = (EditText) findViewById(R.id.signup_edit_confirm_email);
        checkboxRememberAccount = (CheckBox) findViewById(R.id.signup_checkbox_remember_account);
        editPassword = (EditText) findViewById(R.id.signup_edit_password);

        buttonBack = (Button) findViewById(R.id.signup_button_back);
        buttonNext = (Button) findViewById(R.id.signup_button_next);
    }

    private boolean checkFieldFilled() {
        
        if (editFirstname.getText().toString().equals("")) {
            Utility.ToastMessage(this, R.string.message_firstname_empty);
            return false;
        }
        if (editLastname.getText().toString().equals("")) {
            Utility.ToastMessage(this, R.string.message_lastname_empty);
            return false;
        }
        if (editEmail.getText().toString().equals("")) {
            Utility.ToastMessage(this, R.string.message_email_empty);
            return false;
        }
//        if (!Utility.checkEmailFormatInvalid(editEmail.getText().toString())) {
//            Utility.ToastMessage(this, R.string.message_email_invalid);
//            return false;
//        }
        if (!editEmail.getText().toString().equals(editConfirmEmail.getText().toString())) {
            Utility.ToastMessage(this, R.string.message_confirm_email_invalid);
            return false;
        }
        if (editPassword.getText().toString().equals("")) {
            Utility.ToastMessage(this, R.string.message_password_empty);
            return false;
        }
        return true;
    }
    
    private void saveAccount () {
        boolean checked = checkboxRememberAccount.isChecked();
        Utility.setIsRememberAccount(this, checked);
        if (checked)
            Utility.setRememberedAccount(this, editEmail.getText().toString());
    }

    private Button.OnClickListener buttonBackClickListener = new Button.OnClickListener() {
        @Override
        public void onClick(View arg0) {
            finish();
        }
    };

    private Button.OnClickListener buttonNextClickListener = new Button.OnClickListener() {
        @Override
        public void onClick(View arg0) {
            if (!checkFieldFilled())
                return;
            
            saveAccount();

            Intent intent = new Intent(SignUpEntryActivity.this, ProgressingActivity.class);
            intent.putExtra(InternalDefines.BUNDLE_PROGRESS_TYPE, InternalDefines.PROGRESSING_TYPE_SIGNUP);
            intent.putExtra(InternalDefines.ACCOUNT_BUNDLE_FIRSTNAME, editFirstname.getText().toString());
            intent.putExtra(InternalDefines.ACCOUNT_BUNDLE_LASTNAME, editLastname.getText().toString());
            intent.putExtra(InternalDefines.ACCOUNT_BUNDLE_EMAIL, editEmail.getText().toString());
            intent.putExtra(InternalDefines.ACCOUNT_BUNDLE_PASSWORD, editPassword.getText().toString());
            startActivityForResult(intent, 0);
        }

    };
}
