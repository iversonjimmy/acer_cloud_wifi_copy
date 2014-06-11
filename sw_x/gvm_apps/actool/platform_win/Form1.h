#pragma once


namespace actool_win {

	using namespace System;
    using namespace System::IO;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for Form1
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class Form1 : public System::Windows::Forms::Form
	{
	public:
		Form1(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~Form1()
		{
			if (components)
			{
				delete components;
			}
		}
    private: System::Windows::Forms::Label^  label1;
    protected: 
    private: System::Windows::Forms::TextBox^  textBox1;
    private: System::Windows::Forms::Label^  label2;
    private: System::Windows::Forms::Button^  button1;
    private: System::Windows::Forms::Button^  button2;
    private: System::Windows::Forms::TextBox^  textBox2;
    private: System::Windows::Forms::Label^  label3;
	private: System::Windows::Forms::Label^  label4;
	private: System::Windows::Forms::TextBox^  textBox3;

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->textBox1 = (gcnew System::Windows::Forms::TextBox());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->button2 = (gcnew System::Windows::Forms::Button());
			this->textBox2 = (gcnew System::Windows::Forms::TextBox());
			this->label3 = (gcnew System::Windows::Forms::Label());
			this->label4 = (gcnew System::Windows::Forms::Label());
			this->textBox3 = (gcnew System::Windows::Forms::TextBox());
			this->SuspendLayout();
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(12, 55);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(45, 12);
			this->label1->TabIndex = 0;
			this->label1->Text = L"Domain:";
			// 
			// textBox1
			// 
			this->textBox1->Location = System::Drawing::Point(85, 52);
			this->textBox1->Name = L"textBox1";
			this->textBox1->Size = System::Drawing::Size(276, 22);
			this->textBox1->TabIndex = 1;
			// 
			// label2
			// 
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(12, 8);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(284, 24);
			this->label2->TabIndex = 2;
			this->label2->Text = L"This will generate a new configuration for ccd.exe. You will\r\nneed to restart the" 
				L" acerCloud application.\r\n";
			// 
			// button1
			// 
			this->button1->Location = System::Drawing::Point(274, 156);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(75, 21);
			this->button1->TabIndex = 5;
			this->button1->Text = L"Generate";
			this->button1->UseVisualStyleBackColor = true;
			this->button1->Click += gcnew System::EventHandler(this, &Form1::button1_Click);
			// 
			// button2
			// 
			this->button2->Location = System::Drawing::Point(164, 156);
			this->button2->Name = L"button2";
			this->button2->Size = System::Drawing::Size(93, 21);
			this->button2->TabIndex = 6;
			this->button2->Text = L"Reset Default";
			this->button2->UseVisualStyleBackColor = true;
			this->button2->Click += gcnew System::EventHandler(this, &Form1::button2_Click);
			// 
			// textBox2
			// 
			this->textBox2->Location = System::Drawing::Point(85, 80);
			this->textBox2->Name = L"textBox2";
			this->textBox2->Size = System::Drawing::Size(276, 22);
			this->textBox2->TabIndex = 3;
			// 
			// label3
			// 
			this->label3->AutoSize = true;
			this->label3->Location = System::Drawing::Point(11, 85);
			this->label3->Name = L"label3";
			this->label3->Size = System::Drawing::Size(38, 12);
			this->label3->TabIndex = 4;
			this->label3->Text = L"Group:";
			// 
			// label4
			// 
			this->label4->AutoSize = true;
			this->label4->Location = System::Drawing::Point(12, 113);
			this->label4->Name = L"label4";
			this->label4->Size = System::Drawing::Size(67, 12);
			this->label4->TabIndex = 7;
			this->label4->Text = L"Brand Name:";
			this->label4->Click += gcnew System::EventHandler(this, &Form1::label4_Click);
			// 
			// textBox3
			// 
			this->textBox3->Location = System::Drawing::Point(85, 110);
			this->textBox3->Name = L"textBox3";
			this->textBox3->Size = System::Drawing::Size(276, 22);
			this->textBox3->TabIndex = 8;
			// 
			// Form1
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 12);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(373, 199);
			this->Controls->Add(this->textBox3);
			this->Controls->Add(this->label4);
			this->Controls->Add(this->textBox2);
			this->Controls->Add(this->label3);
			this->Controls->Add(this->button2);
			this->Controls->Add(this->button1);
			this->Controls->Add(this->label2);
			this->Controls->Add(this->textBox1);
			this->Controls->Add(this->label1);
			this->Name = L"Form1";
			this->Text = L"Acer Cloud Tool";
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion

    private:

        System::Void button1_Click(System::Object^  sender, System::EventArgs^  e)
        {
            try {
                if (textBox1->Text->Length <= 0 || textBox3->Text->Length <= 0) {
                    MessageBox::Show(L"Please fill in the domain and brand name!");
                    return;
                }

                FileInfo^ info = gcnew FileInfo(Application::ExecutablePath);
                String^ input = File::ReadAllText(info->DirectoryName + L"\\ccd.conf.tmpl");
                String^ output = input->Replace(L"${DOMAIN}", textBox1->Text);
                output = output->Replace(L"${GROUP}", textBox2->Text);
                String^ directory1 = Environment::GetFolderPath(Environment::SpecialFolder::LocalApplicationData);
                String^ directory2 = directory1;
                directory1 += L"\\iGware\\SyncAgent\\conf";
                directory2 += L"\\AOP SDK\\Acer Infra\\" + textBox3->Text + L"\\SyncAgent\\conf";
                Directory::CreateDirectory(directory1);
                Directory::CreateDirectory(directory2);
                File::WriteAllText(directory1 + L"\\ccd.conf", output);
                File::WriteAllText(directory2 + L"\\ccd.conf", output);

                MessageBox::Show(
                    L"Config generated successfully. Please restart the AcerCloud application.",
                    L"",
                    MessageBoxButtons::OK);
            } catch (Exception^ ex) {
                MessageBox::Show(ex->Message);
            }
        }

        System::Void button2_Click(System::Object^  sender, System::EventArgs^  e)
        {
            try {
				if (textBox3->Text->Length <= 0) {
                    MessageBox::Show(L"Please fill in brand name!");
                    return;
                }

                String^ fileName = Environment::GetFolderPath(Environment::SpecialFolder::LocalApplicationData);
                fileName += L"\\iGware\\SyncAgent\\conf\\ccd.conf";
                File::Delete(fileName);

                fileName = Environment::GetFolderPath(Environment::SpecialFolder::LocalApplicationData);
                fileName += L"\\AOP SDK\\Acer Infra\\" + textBox3->Text + L"\\SyncAgent\\conf\\ccd.conf";
                File::Delete(fileName);

                MessageBox::Show(
                    L"Config reset to default state. Please restart the acerCloud application.",
                    L"",
                    MessageBoxButtons::OK);
            } catch (Exception^ ex) {
                MessageBox::Show(ex->Message);
            }
        }
private: System::Void label4_Click(System::Object^  sender, System::EventArgs^  e) {
		 }
};
}

