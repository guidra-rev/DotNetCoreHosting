namespace WinFormsSample
{
    partial class Form1
    {
        /// <summary>
        ///  Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        ///  Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        ///  Required method for Designer support - do not modify
        ///  the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.label1 = new System.Windows.Forms.Label();
            this.lblRuntimeVersion = new System.Windows.Forms.Label();
            this.btnDebug = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point);
            this.label1.Location = new System.Drawing.Point(12, 9);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(76, 21);
            this.label1.TabIndex = 0;
            this.label1.Text = "Runtime :";
            // 
            // lblRuntimeVersion
            // 
            this.lblRuntimeVersion.AutoSize = true;
            this.lblRuntimeVersion.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point);
            this.lblRuntimeVersion.Location = new System.Drawing.Point(94, 9);
            this.lblRuntimeVersion.Name = "lblRuntimeVersion";
            this.lblRuntimeVersion.Size = new System.Drawing.Size(71, 21);
            this.lblRuntimeVersion.TabIndex = 1;
            this.lblRuntimeVersion.Text = "loading...";
            // 
            // btnDebug
            // 
            this.btnDebug.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point);
            this.btnDebug.Location = new System.Drawing.Point(13, 50);
            this.btnDebug.Name = "btnDebug";
            this.btnDebug.Size = new System.Drawing.Size(75, 31);
            this.btnDebug.TabIndex = 2;
            this.btnDebug.Text = "Debug";
            this.btnDebug.UseVisualStyleBackColor = true;
            this.btnDebug.Click += new System.EventHandler(this.btnDebug_Click);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(336, 105);
            this.Controls.Add(this.btnDebug);
            this.Controls.Add(this.lblRuntimeVersion);
            this.Controls.Add(this.label1);
            this.Name = "Form1";
            this.Text = "Form1";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label lblRuntimeVersion;
        private System.Windows.Forms.Button btnDebug;
    }
}

