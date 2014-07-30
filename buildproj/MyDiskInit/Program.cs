using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace Build
{
    class Program
    {
        static void Main(string[] args)
        {
            createMyDisk(MachinePara.DiskPath);
            createMyBoot(MachinePara.BootPath);
            Run r = new Run();
            r.Begin();
        }
        static void createMyDisk(string diskname)
        {
            FileStream fs = System.IO.File.Create(diskname);
            fs.Seek(MachinePara.Disk_Size * MachinePara.Block_Size, SeekOrigin.Begin);
            byte i = 1;
            fs.WriteByte(i);
            fs.Close();
        }
        static void createMyBoot(string bootname)
        {
            FileStream fs = System.IO.File.Create(bootname);
            fs.Seek(MachinePara.Boot_Size * MachinePara.Block_Size - 1, SeekOrigin.Begin);
            byte i = 1;
            fs.WriteByte(i);
            fs.Close();
        }
    }
}

