#pragma once

using namespace  System;
using namespace System::ComponentModel;
using namespace System::Drawing;
using namespace System::Windows::Forms;
 
public ref class WrapLabel : public System::Windows::Forms::Label
{

public:
	WrapLabel()
    {
		this->AutoSize = false;
    }

	~WrapLabel()
	{}
    

protected:
     void OnResize(EventArgs e) 
    {

		this->FitToContents();
    }
 
    void OnTextChanged(EventArgs e)
    {
      //base.OnTextChanged(e);
 
      this->FitToContents();
    }
 
    void FitToContents()
    {
		System::Drawing::Size size;
 
		size = this->GetPreferredSize(System::Drawing::Size(this->Width, 0));
		  
 
		this->Height = size.Height;
    }
 /*
    #endregion  Protected Virtual Methods  
 
    #region  Public Properties  
 
    [DefaultValue(false), Browsable(false), EditorBrowsable(EditorBrowsableState.Never), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]

    #endregion  Public Properties  */
  };
