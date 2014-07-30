﻿using System;

namespace Build
{

    /// <summary>
    /// Inode结构体
    /// </summary>
    public class InodeStr
    {
        public uint _i_mode;
        public int _i_ilink;
        public short _i_uid;
        public short _i_gid;
        public int _i_size;
        public int[] _i_addr = new int[10];
        public int _i_atime;
        public int _i_mtime;

        public InodeStr()
        {
            _i_mode = 0;
            _i_ilink = 0;
            _i_uid = 0;
            _i_gid = 0;
            _i_size = 0;
            for (int i = 0; i < 10; ++i)
            {
                _i_addr[i] = 0;
            }
            _i_atime = 0;
            _i_mtime = 0;
        }
    };

    /// <summary>
    /// Inode区管理类
    /// </summary>
    public class InodeBlock
    {
        private Superblock _initSuper;

        /// <summary>
        /// inode的大小，单位是字节
        /// </summary>
        private const int INODE_SIZE = 64;
        /// <summary>
        /// 直接管理inode数目
        /// </summary>
        private const int DERECTN_INODE = 100;
        /// <summary>
        /// inode区的起始块号
        /// </summary>
        //private static int INODE_START = MachinePara.BootAndKernelSize + MachinePara.SuperBlockSize;
        private static int INODE_START = MachinePara.Boot_Size + MachinePara.SuperBlockSize;
        /// <summary>
        /// 磁盘文件
        /// </summary>
        private Disk _diskFile;
        /// <summary>
        /// 构造函数
        /// </summary>
        /// <param name="tsb"></param>
        /// <param name="diskFile"></param>
        public InodeBlock(Superblock tsb, Disk diskFile)
        {
            _initSuper = tsb;
            _diskFile = diskFile;
        }

        #region 初始化inode区
        public void initInodeManager()
        {
            //初始化所有的inode，使其i-mode == 0
            CreatInode();
            //填写直接控制的100个空闲inode
            CreatMngArray();
        }

        //在磁盘的inode区中创建inode
        private void CreatInode()
        {
            //将i_mode == 0写入inode结构中，表示该inode空闲
            uint i_mode = 0;
            byte[] writeinto;

            _diskFile.OpenFile();
            //从第二个盘块起是inode区
            for (int i = 0 + INODE_START; i < _initSuper._s_isize + INODE_START; ++i)
            {
                for (int j = 0; j < MachinePara.Block_Size; j += INODE_SIZE)
                {
                    //找到要写入的位置
                    _diskFile.SeekFilePosition(_diskFile.ConvertPosition(i, j), System.IO.SeekOrigin.Begin);
                    //转换成流
                    writeinto = Helper.Struct2Bytes(i_mode);
                    //写入初始化的值，即将i_mode == 0写入
                    _diskFile.WriteFile(ref writeinto, 0, 4);
                }
            }
            _diskFile.CloseFile();
        }

        //初始化SuperBlock中的inode管理数组
        private void CreatMngArray()
        {
            //最后一个inode的编号
            int inodetail = _initSuper._s_isize * (MachinePara.Block_Size / INODE_SIZE) - 1;

            for (int i = 0; i < DERECTN_INODE - 1; ++i, --inodetail)
            {
                //超级块的s_inode数组中存放的只是inode的编号
                _initSuper._s_inode[i] = inodetail;
            }
            //rootdir
            _initSuper._s_inode[DERECTN_INODE - 1] = 0;

            //直接控制inode数量
            _initSuper._s_ninode = DERECTN_INODE;    /* spb.s_inode[spb.s_ninode++] = ino */

            _initSuper.UpdateSuperBlockToDisk();
        }
        #endregion

        #region inode操作
        /// <summary>
        /// 清空inode中的addr数组信息
        /// </summary>
        /// <param name="id"></param>
        public void CleanInodeAddr(InodeStr id)
        {
            for (int i = 0; i < 10; ++i)
            {
                id._i_addr[i] = 0;
            }
        }

        /// <summary>
        /// 得到一个inode
        /// </summary>
        /// <returns></returns>
        public int FetchFreeInode()
        {
            //inode溢出

            int inodeTail = _initSuper._s_isize * (MachinePara.Block_Size / INODE_SIZE) - 1;

            if (_initSuper._s_ninode <= 0)
            {
                int i;
                //如缺少inode管理里缺少空闲节点，则去inode取抓取足够的空节点
                for (i = inodeTail; _initSuper._s_ninode < DERECTN_INODE && i >= 0; i--)
                {
                    if (IsInodeFree(i) == 1)
                        _initSuper._s_inode[_initSuper._s_ninode++] = i;
                }
                if (_initSuper._s_ninode == 0 && i < 0)
                {
                    throw (new Exception("没有空闲inode"));
                }
            }
            _initSuper._s_ninode--;
            _initSuper.UpdateSuperBlockToDisk();

            return _initSuper._s_inode[_initSuper._s_ninode];
        }

        /// 判断inode是否空闲
        public int IsInodeFree(int no)
        {
            byte[] readFrom = new byte[4];
            int startPosition = INODE_START * MachinePara.Block_Size + INODE_SIZE * no;
            _diskFile.OpenFile();
            _diskFile.SeekFilePosition(startPosition, System.IO.SeekOrigin.Begin);
            _diskFile.ReadFile(ref readFrom, 0, 4);
            _diskFile.CloseFile();
            if ((uint)Helper.Bytes2Struct(readFrom, typeof(uint)) == 0)
                return 1;
            else
                return 0;
        }

        ///释放一个inode
        public void FreeInode(int no)
        {
            InodeStr istr = new InodeStr();

            UpdateInodeToDisk(istr, no);
            if (_initSuper._s_ninode < 100)
                _initSuper._s_inode[_initSuper._s_ninode++] = no;

            _initSuper.UpdateSuperBlockToDisk();
        }

        /// <summary>
        /// 从硬盘获取inode值
        /// </summary>
        /// <param name="istr"></param>
        /// <param name="no"></param>
        public void GetInodeFromDisk(InodeStr istr, int no)
        {
            byte[] readFrom;
            int startPosition;

            _diskFile.OpenFile();
            //inode所在的流的位置
            startPosition = INODE_START * MachinePara.Block_Size + INODE_SIZE * no;
            _diskFile.SeekFilePosition(startPosition, System.IO.SeekOrigin.Begin);

            //读入i_mode
            readFrom = new byte[System.Runtime.InteropServices.Marshal.SizeOf(istr._i_mode)];
            _diskFile.ReadFile(ref readFrom, 0, System.Runtime.InteropServices.Marshal.SizeOf(istr._i_mode));
            istr._i_mode = (uint)Helper.Bytes2Struct(readFrom, typeof(uint));

            //读入i_ilink
            readFrom = new byte[System.Runtime.InteropServices.Marshal.SizeOf(istr._i_ilink)];
            _diskFile.ReadFile(ref readFrom, 0, System.Runtime.InteropServices.Marshal.SizeOf(istr._i_ilink));
            istr._i_ilink = (int)Helper.Bytes2Struct(readFrom, typeof(int));

            //读入i_uid
            readFrom = new byte[System.Runtime.InteropServices.Marshal.SizeOf(istr._i_uid)];
            _diskFile.ReadFile(ref readFrom, 0, System.Runtime.InteropServices.Marshal.SizeOf(istr._i_uid));
            istr._i_uid = (short)Helper.Bytes2Struct(readFrom, typeof(short));

            //读入i_gid
            readFrom = new byte[System.Runtime.InteropServices.Marshal.SizeOf(istr._i_gid)];
            _diskFile.ReadFile(ref readFrom, 0, System.Runtime.InteropServices.Marshal.SizeOf(istr._i_gid));
            istr._i_gid = (short)Helper.Bytes2Struct(readFrom, typeof(short));

            //读入i_size
            readFrom = new byte[System.Runtime.InteropServices.Marshal.SizeOf(istr._i_size)];
            _diskFile.ReadFile(ref readFrom, 0, System.Runtime.InteropServices.Marshal.SizeOf(istr._i_size));
            istr._i_size = (int)Helper.Bytes2Struct(readFrom, typeof(int));

            //读入i_addr
            readFrom = new byte[4];
            for (int i = 0; i < 10; i++)
            {
                _diskFile.ReadFile(ref readFrom, 0, 4);
                istr._i_addr[i] = (int)Helper.Bytes2Struct(readFrom, typeof(int));
            }

            //读入i_atime
            readFrom = new byte[System.Runtime.InteropServices.Marshal.SizeOf(istr._i_atime)];
            _diskFile.ReadFile(ref readFrom, 0, System.Runtime.InteropServices.Marshal.SizeOf(istr._i_atime));
            istr._i_atime = (int)Helper.Bytes2Struct(readFrom, typeof(int));

            //读入i_atime
            readFrom = new byte[System.Runtime.InteropServices.Marshal.SizeOf(istr._i_mtime)];
            _diskFile.ReadFile(ref readFrom, 0, System.Runtime.InteropServices.Marshal.SizeOf(istr._i_mtime));
            istr._i_mtime = (int)Helper.Bytes2Struct(readFrom, typeof(int));

            _diskFile.CloseFile();
        }

        /// <summary>
        /// 设置inode值并保存到硬盘
        /// </summary>
        /// <param name="istr">需写入的inodestr</param>
        /// <param name="no">inode编号</param>
        public void UpdateInodeToDisk(InodeStr istr, int no)
        {
            byte[] writeTo;
            int startPosition;

            _diskFile.OpenFile();
            //inode所在的流的位置
            startPosition = INODE_START * MachinePara.Block_Size + INODE_SIZE * no;
            _diskFile.SeekFilePosition(startPosition, System.IO.SeekOrigin.Begin);
            //写入i_mode
            writeTo = Helper.Struct2Bytes(istr._i_mode);
            _diskFile.WriteFile(ref writeTo, 0, 4);
            //写入i_ilink
            writeTo = Helper.Struct2Bytes(istr._i_ilink);
            _diskFile.WriteFile(ref writeTo, 0, 4);
            //写入i_uid
            writeTo = Helper.Struct2Bytes(istr._i_uid);
            _diskFile.WriteFile(ref writeTo, 0, 2);
            //写入i_gid
            writeTo = Helper.Struct2Bytes(istr._i_gid);
            _diskFile.WriteFile(ref writeTo, 0, 2);
            //写入i_size
            writeTo = Helper.Struct2Bytes(istr._i_size);
            _diskFile.WriteFile(ref writeTo, 0, 4);
            //写入i_addr
            for (int i = 0; i < 10; ++i)
            {
                writeTo = Helper.Struct2Bytes(istr._i_addr[i]);
                _diskFile.WriteFile(ref writeTo, 0, 4);
            }
            //写入i_atime
            writeTo = Helper.Struct2Bytes(istr._i_atime);
            _diskFile.WriteFile(ref writeTo, 0, 4);
            //写入i_mtime
            writeTo = Helper.Struct2Bytes(istr._i_mtime);
            _diskFile.WriteFile(ref writeTo, 0, 4);

            _diskFile.CloseFile();
        }
        #endregion
    }
}
