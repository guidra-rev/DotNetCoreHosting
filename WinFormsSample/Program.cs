using System;
using System.Windows.Forms;

namespace WinFormsSample
{
    static class Program
    {
        // Do not change the signature
        [STAThread]
        public static int HostEntryPoint(IntPtr arg, int argLength)
        {
            InternalMain();

            return 0;
        }

        [STAThread]
        public static void Main()
        {
            InternalMain();
        }

        [STAThread]
        private static void InternalMain()
        {
            Application.SetHighDpiMode(HighDpiMode.SystemAware);
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new Form1());
        }
    }
}
