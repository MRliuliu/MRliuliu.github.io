/*
 * =====================================================================================
 *
 *       Filename:  ringbuf.h
 *
 *    Description:  data porcess / save
 *
 *        Version:  1.0
 *        Created:  05/06/2013 10:58:11 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Changqing,Zhao ,changqing.1230@163.com
 *        Company:  None
 *
 * =====================================================================================
 */
#ifndef _RINGBUF_H_
#define _RINGBUF_H_

#include <string.h>

#define cycle_is_empty(pcy)		((pcy)->wpos == (pcy)->rpos)
#define IOV_SIZE 131072

////////////////////////////////////////////////////////////////////////
//数据缓存结构
typedef struct iovect_s{
	unsigned char *buf;
	volatile int len;
	int size;
}iovect_t;

////////////////////////////////////////////////////////////////////////
//循环存储结构体 
typedef struct ringbuf_s{
	iovect_t iov[IOV_SIZE];
	int size;
	volatile int rpos, wpos;
	int overflow;
	/**added by yjg**/
	/**为了方便使用，将每一个块的大小都固定为一个值**/
#if 1
	int unitsize;
#endif
	/**added end**/
}ringbuf_t;

static inline void clear_buf(ringbuf_t *pcy)
{
	pcy->rpos = pcy->wpos = 0;
}

static inline void set_rpos(ringbuf_t *pcy, int offset)
{
	pcy->rpos += offset;
	pcy->rpos %= pcy->size;
}

static inline void set_wpos(ringbuf_t *pcy, int offset)
{
	pcy->wpos += offset;
	pcy->wpos %= pcy->size;
}

static inline int is_overflow(ringbuf_t *pcy)
{
	return pcy->overflow;
}

/* get the write buf, to write */
static inline iovect_t *get_write_iov(ringbuf_t *pcy)
{
	return &pcy->iov[pcy->wpos];
}

static inline void put_iov(ringbuf_t *pcy)
{
	set_wpos(pcy, 1);
	if( pcy->wpos == pcy->rpos )
	{
		pcy->overflow = 1;
		set_rpos(pcy, 1);
	}
}

static inline unsigned char *get_write_buf(ringbuf_t *pcy)
{
	return pcy->iov[pcy->wpos].buf;
}

static inline int get_write_buf_len(ringbuf_t *pcy)
{
	return pcy->iov[pcy->wpos].len;
}

static inline unsigned char *get_lastwrite_buf(ringbuf_t *pcy)
{
	int index = 0;
	if( pcy->wpos == 0 )
		return pcy->iov[pcy->size - 1].buf;

	index = pcy->wpos - 1;
	return pcy->iov[index].buf;
}

static inline int get_lastwrite_buflen(ringbuf_t *pcy)
{
	int index = 0;
	if( pcy->wpos == 0 )
		return pcy->iov[pcy->size - 1].len;

	index = pcy->wpos - 1;
	return pcy->iov[index].len;
}

static inline unsigned char *get_read_buf(ringbuf_t *pcy)
{
	return pcy->iov[pcy->rpos].buf;
}

static inline int get_read_buf_len(ringbuf_t *pcy)
{
	return pcy->iov[pcy->rpos].len;
}

static inline void put_buf(ringbuf_t *pcy, void *buf, int len)
{
	if(buf == pcy->iov[pcy->wpos].buf)
	{
		pcy->iov[pcy->wpos].len = len;
		set_wpos(pcy, 1);
		/* clean new buf len */
		pcy->iov[pcy->wpos].len = 0;
#if 0
		if( pcy->wpos == pcy->rpos )
		{
            printf("overflow\n");
			pcy->overflow = 1;
			set_rpos(pcy, 1);
		}
#endif
	}else{
		int _len = len;
		for(; len > 0; len -= _len)
		{
			if( len > pcy->iov[pcy->wpos].size )
				_len = pcy->iov[pcy->wpos].size;
			memcpy(pcy->iov[pcy->wpos].buf, buf, _len);
			pcy->iov[pcy->wpos].len = _len;
			set_wpos(pcy,1);

			if(pcy->wpos == pcy->rpos)
			{
				pcy->overflow = 1;
				set_rpos(pcy, 1);
			}
		}
	}
}

static inline int get_buf(ringbuf_t *pcy, void *buf, int len)
{
	int _len = len;
	int ret = 0;

	if( buf != pcy->iov[pcy->rpos].buf )
	{
		for(; len > 0; len -= _len)
		{
			if( len > pcy->iov[pcy->rpos].len )
				_len = pcy->iov[pcy->rpos].len;
			memcpy(buf, pcy->iov[pcy->rpos].buf, _len);
			ret += _len;
			set_rpos(pcy, 1);
			if( pcy->wpos == pcy->rpos )
			{
				goto out;
			}
		}
	}else{
	/* this is must , 2013-05-23 20:57 zhaocq*/
		pcy->iov[pcy->rpos].len = 0;
		set_rpos(pcy, 1);
		if( pcy->wpos == pcy->rpos )
		{
			goto out;
		}
	}

	return ret;
out:
	if( is_overflow(pcy) )
	{
		pcy->overflow = 0;
	}

	return ret;
}

static inline int get_iov_size(ringbuf_t* pcy)
{
	return pcy->iov[0].size;
}

static inline int get_current_size(ringbuf_t* pcy)
{
	int tmp = ( pcy->wpos >= pcy->rpos ) ? (pcy->wpos-pcy->rpos) : (pcy->size-(pcy->rpos-pcy->wpos));
	if ( tmp > pcy->size )
	{
		return 0;
	}
	return tmp;
}


/******************************基于上面的接口，封装一套更方便使用的接口**********/
/*******************************Added by Yjg*************************************/

// 获取缓冲区的单元块大小
static inline int rb_get_unitsize(ringbuf_t* prb)
{
    if ( prb )
        return prb->unitsize;

    return -1;
}

// 缓冲区是否是满的
static inline int rb_isfull(ringbuf_t *prb)
{
    if ( prb )
        return (((prb->wpos+1)%prb->size) == prb->rpos);
	return -1;
}

// 缓冲区是否是空的
static inline int rb_isempty(ringbuf_t *prb)
{
    if ( prb )
        return cycle_is_empty(prb);
	return -1;
}

// 销毁一个缓冲区
static inline void rb_destroy(ringbuf_t *prb)
{
    if ( NULL == prb )
    {
        return;
    }
    int i = 0;
    // 释放每个块
    for (i=0; i<IOV_SIZE; i++)
    {
        if ( NULL != prb->iov[i].buf )
            free(prb->iov[i].buf);
    }

    free(prb);
    prb = NULL;

	return;
}

// 创建一个缓冲区
static inline ringbuf_t *rb_create(int number, int unitsize)
{
    ringbuf_t *prb = NULL;

    // 缓冲区块数
    if ( IOV_SIZE < number )
    {
        goto err;
    }

    prb = (ringbuf_t *)malloc(sizeof(ringbuf_t));
    if ( NULL == prb )
    {
        goto err;
    }

    memset(prb, 0, sizeof(ringbuf_t));
    prb->size = number;
    prb->unitsize = unitsize;

    int i = 0;
    // 分配每个块
    for (i=0; i<prb->size; i++)
    {
        prb->iov[i].buf = (unsigned char *)malloc(prb->unitsize);
        if ( NULL == prb->iov[i].buf )
        {
            goto err;
        }
        prb->iov[i].len = 0;
        prb->iov[i].size = prb->unitsize;
    }

	return prb;
err:
    rb_destroy(prb);
    return NULL;
}

// 向缓冲区中放入一个块
static inline int rb_put(ringbuf_t* prb, char *buf, int len)
{
    unsigned char *tmpbuf = NULL;

    if ( NULL == prb || NULL == buf )
    {
        goto err;
    }
    if ( len > prb->unitsize )
    {
        goto err;
    }

    if ( rb_isfull(prb) )
    {
        goto err;
    }

    tmpbuf = get_write_buf(prb);
    memcpy(tmpbuf, buf, len);
    put_buf(prb, tmpbuf, len);

    return 0;
err:
    return -1;
}

// 从缓冲区中读取一个块
static inline int rb_get(ringbuf_t* prb, unsigned char *buf, int len)
{
    unsigned char *tmpbuf = NULL;
    int tmplen = 0;
    if ( NULL == prb || NULL == buf )
    {
        goto err;
    }

    if ( rb_isempty(prb) )
    {
        goto err;
    }

    tmpbuf = get_read_buf(prb);
    tmplen = get_read_buf_len(prb);
    memcpy(buf, tmpbuf, (len >= tmplen)?tmplen:len);
    get_buf(prb, tmpbuf, tmplen);

    return (len >= tmplen)?tmplen:len;
err:
    return -1;
}

/***************Added End********************************************************/

#endif
