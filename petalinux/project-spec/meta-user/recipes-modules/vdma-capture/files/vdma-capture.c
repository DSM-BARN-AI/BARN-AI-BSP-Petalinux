/*  vdma-capture.c - VDMA S2MM dmaengine client (BARN AI BSP)
 *
 *  axi_vdma_0(레지스터 직접 안 만짐)의 dmaengine 채널을 빌려서
 *  S2MM(캡처) 방향으로 한 프레임(64x48x3=9216B)을 DDR로 받는 클라이언트.
 *  echo 1 > .../capture 로 트리거.
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_dma.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>

#define DRIVER_NAME   "vdma-capture"
#define FRAME_WIDTH   64
#define FRAME_HEIGHT  48
#define BYTES_PER_PX  3
#define STRIDE        (FRAME_WIDTH * BYTES_PER_PX)   /* 192 */
#define FRAME_SIZE    (STRIDE * FRAME_HEIGHT)          /* 9216 */

struct vdma_capture_local {
        struct dma_chan *chan;
        void *buf;
        dma_addr_t buf_phys;
        struct completion done;
};

static void vdma_capture_done_cb(void *param)
{
        struct vdma_capture_local *lp = param;
        complete(&lp->done);
}

/* echo 1 > capture 로 트리거 */
static ssize_t capture_store(struct device *dev,
                             struct device_attribute *attr,
                             const char *buf, size_t count)
{
        struct vdma_capture_local *lp = dev_get_drvdata(dev);
        struct dma_interleaved_template *xt;
        struct dma_async_tx_descriptor *tx;
        dma_cookie_t cookie;

        xt = kzalloc(sizeof(*xt) + sizeof(struct data_chunk), GFP_KERNEL);
        if (!xt)
                return -ENOMEM;

        xt->dir = DMA_DEV_TO_MEM;
        xt->src_start = 0;
        xt->dst_start = lp->buf_phys;
        xt->dst_sgl = true;
        xt->numf = FRAME_HEIGHT;
        xt->sgl[0].size = STRIDE;
        xt->sgl[0].icg = 0;
        xt->frame_size = 1;

        tx = dmaengine_prep_interleaved_dma(lp->chan, xt, DMA_PREP_INTERRUPT);
        kfree(xt);
        if (!tx) {
                dev_err(dev, "prep_interleaved failed\n");
                return -EIO;
        }

        tx->callback = vdma_capture_done_cb;
        tx->callback_param = lp;
        reinit_completion(&lp->done);

        cookie = dmaengine_submit(tx);
        if (dma_submit_error(cookie))
                return -EIO;

        dma_async_issue_pending(lp->chan);
        wait_for_completion_timeout(&lp->done, msecs_to_jiffies(1000));

        dev_info(dev, "capture done, buf=%p phys=%pad\n", lp->buf, &lp->buf_phys);
        return count;
}
static DEVICE_ATTR_WO(capture);

static int vdma_capture_probe(struct platform_device *pdev)
{
        struct device *dev = &pdev->dev;
        struct vdma_capture_local *lp;
        int rc;

        lp = devm_kzalloc(dev, sizeof(*lp), GFP_KERNEL);
        if (!lp)
                return -ENOMEM;

        lp->chan = dma_request_chan(dev, "s2mm");
        if (IS_ERR(lp->chan)) {
                dev_err(dev, "failed to request s2mm channel (may be -EPROBE_DEFER)\n");
                return PTR_ERR(lp->chan);
        }

        lp->buf = dma_alloc_coherent(dev, FRAME_SIZE, &lp->buf_phys, GFP_KERNEL);
        if (!lp->buf) {
                dma_release_channel(lp->chan);
                return -ENOMEM;
        }

        init_completion(&lp->done);
        dev_set_drvdata(dev, lp);

        rc = device_create_file(dev, &dev_attr_capture);
        if (rc)
                dev_err(dev, "sysfs create failed\n");

        dev_info(dev, "vdma-capture probe done, buf_phys=%pad\n", &lp->buf_phys);
        return 0;
}

static void vdma_capture_remove(struct platform_device *pdev)
{
        struct vdma_capture_local *lp = dev_get_drvdata(&pdev->dev);
        device_remove_file(&pdev->dev, &dev_attr_capture);
        dma_free_coherent(&pdev->dev, FRAME_SIZE, lp->buf, lp->buf_phys);
        dma_release_channel(lp->chan);
}

static const struct of_device_id vdma_capture_of_match[] = {
        { .compatible = "barn,vdma-capture", },
        { },
};
MODULE_DEVICE_TABLE(of, vdma_capture_of_match);

static struct platform_driver vdma_capture_driver = {
        .driver = {
                .name = DRIVER_NAME,
                .of_match_table = vdma_capture_of_match,
        },
        .probe  = vdma_capture_probe,
        .remove = vdma_capture_remove,
};
module_platform_driver(vdma_capture_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BARN AI");
MODULE_DESCRIPTION("VDMA S2MM capture client for TPG test");
