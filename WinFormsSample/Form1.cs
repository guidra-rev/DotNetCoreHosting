using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System;

namespace WinFormsSample
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, System.EventArgs e)
        {
            var bitness = Environment.Is64BitProcess ? "64" : "32";
            lblRuntimeVersion.Text = $"{RuntimeInformation.FrameworkDescription} ({bitness}-bit)";
        }

        private void btnDebug_Click(object sender, System.EventArgs e)
        {
            Debugger.Launch();
        }
    }
}
