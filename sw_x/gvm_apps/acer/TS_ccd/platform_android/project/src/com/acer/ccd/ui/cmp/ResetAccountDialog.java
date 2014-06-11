/*
 *  Copyright 2011 Acer Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER INC.
 */

package com.acer.ccd.ui.cmp;

import com.acer.ccd.R;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

public class ResetAccountDialog extends AlertDialog {

    private EditText mEditEmail;

    public ResetAccountDialog(Context context) {
        super(context);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        
        View view = getLayoutInflater().inflate(R.layout.reset_account_dialog, null);
        mEditEmail = (EditText) view.findViewById(R.id.dialog_reset_edit_email);
        mEditEmail.addTextChangedListener(mEmailTextWatcher);
        setView(view);

        setButton(BUTTON_NEGATIVE, getContext().getString(android.R.string.cancel),
                new OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {

                    }
                });
        super.onCreate(savedInstanceState);
        getButton(BUTTON_POSITIVE).setEnabled(false);
    }

    public EditText getEditEmail() {
        return mEditEmail;
    }
    
    private boolean validate(CharSequence s) {
        return !s.toString().equals("");
    }

    private TextWatcher mEmailTextWatcher = new TextWatcher() {

        @Override
        public void afterTextChanged(Editable arg0) {
            // TODO Auto-generated method stub
            
        }

        @Override
        public void beforeTextChanged(CharSequence arg0, int arg1, int arg2, int arg3) {
            // TODO Auto-generated method stub
            
        }

        @Override
        public void onTextChanged(CharSequence s, int start, int count, int after) {
            Button button = getButton(BUTTON_POSITIVE);
            button.setEnabled(validate(s));
        }
        
    };

}
