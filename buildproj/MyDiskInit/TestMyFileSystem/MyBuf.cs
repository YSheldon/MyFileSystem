using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MyDiskInit.TestMyFileSystem
{
    class Buf
    {
        public int blkno;
        public char[] buffer;
        public Buf nextBuf;
        public Buf()
        {
            buffer = new char[512];
            nextBuf = null;
        }
    }

    class BufferManager
    {

	    /* static const member */
	    static const int NBUF = 15;			/* 缓存控制块、缓冲区的数量 */
	    static const int BUFFER_SIZE = 512;	/* 缓冲区大小。 以字节为单位 */
        Buf bFreeList;						/* 自由缓存队列控制块 */

        BufferManager()
        {
            bFreeList = new Buf();
            Buf temp = bFreeList;
            for(int i = 0; i < BUFFER_SIZE; i++)
            {
                temp.nextBuf = new Buf();
                temp = temp.nextBuf;
            }
        }

        /* 申请一块缓存，用于读写设备dev上的字符块blkno。*/
        public Buf GetBlk(int blkno)
        {
            Buf temp = bFreeList;
            while(temp.nextBuf != null)
            {
                if (temp.nextBuf.blkno == blkno)
                    break;
            }
            if (temp.nextBuf == null)
                temp = bFreeList;
            Buf reBuf = temp.nextBuf;
            temp.nextBuf = reBuf.nextBuf;
            return reBuf;
        }

        /* 释放缓存控制块buf */
	    public void Brelse(Buf bp);
        {
            bp.nextBuf = null;
        }		
	    public Buf Bread(int blkno);				/* 读一个磁盘块。blkno为目标磁盘块逻辑块号。 */
	    public void Bwrite(Buf bp);				/* 写一个磁盘块 */

    }
}
