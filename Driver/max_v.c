#include <linux/spi.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/delay.h>

static int maxv_probe(struct spi_device *spi)
{
	struct maxv_data	*maxv = NULL;
	struct spi_eeprom	chip;
	int			err;
	int			sr;
	int			addrlen;

	/* Chip description */
	if (!spi->dev.platform_data) {
		err = maxv_fw_to_chip(&spi->dev, &chip);
		if (err)
			return err;
	} else
		chip = *(struct spi_eeprom *)spi->dev.platform_data;

	/* For now we only support 8/16/24 bit addressing */
	if (chip.flags & EE_ADDR1)
		addrlen = 1;
	else if (chip.flags & EE_ADDR2)
		addrlen = 2;
	else if (chip.flags & EE_ADDR3)
		addrlen = 3;
	else {
		dev_dbg(&spi->dev, "unsupported address type\n");
		return -EINVAL;
	}

	/* Ping the chip ... the status register is pretty portable,
	 * unlike probing manufacturer IDs.  We do expect that system
	 * firmware didn't write it in the past few milliseconds!
	 */
    /*
	sr = spi_w8r8(spi, MAXV_RDSR);
	if (sr < 0 || sr & MAXV_SR_nRDY) {
		dev_dbg(&spi->dev, "rdsr --> %d (%02x)\n", sr, sr);
		return -ENXIO;
	}
    */
	maxv = devm_kzalloc(&spi->dev, sizeof(struct maxv_data), GFP_KERNEL);
	if (!maxv)
		return -ENOMEM;

	mutex_init(&maxv->lock);
	maxv->chip = chip;
	maxv->spi = spi;
	spi_set_drvdata(spi, maxv);
	maxv->addrlen = addrlen;

	maxv->nvmem_config.name = dev_name(&spi->dev);
	maxv->nvmem_config.dev = &spi->dev;
	maxv->nvmem_config.read_only = chip.flags & EE_READONLY;
	maxv->nvmem_config.root_only = true;
	maxv->nvmem_config.owner = THIS_MODULE;
	maxv->nvmem_config.compat = true;
	maxv->nvmem_config.base_dev = &spi->dev;
	maxv->nvmem_config.reg_read = maxv_ee_read;
	maxv->nvmem_config.reg_write = maxv_ee_write;
	maxv->nvmem_config.priv = maxv;
	maxv->nvmem_config.stride = 4;
	maxv->nvmem_config.word_size = 1;
	maxv->nvmem_config.size = chip.byte_len;

	maxv->nvmem = nvmem_register(&maxv->nvmem_config);
	if (IS_ERR(maxv->nvmem))
		return PTR_ERR(maxv5->nvmem);

	dev_info(&spi->dev, "%d %s %s eeprom%s, pagesize %u\n",
		(chip.byte_len < 1024) ? chip.byte_len : (chip.byte_len / 1024),
		(chip.byte_len < 1024) ? "Byte" : "KByte",
		maxv->chip.name,
		(chip.flags & EE_READONLY) ? " (readonly)" : "",
		maxv->chip.page_size);
	return 0;
}

static int maxv_remove(struct spi_device *spi)
{
	struct maxv_data	*maxv5;

	maxv = spi_get_drvdata(spi);
	nvmem_unregister(maxv->nvmem);

	return 0;
}

/*-------------------------------------------------------------------------*/

static const struct of_device_id maxv_of_match[] = {
	{ .compatible = "maxv" },
	{ }
};
MODULE_DEVICE_TABLE(of, maxv_of_match);

static struct spi_driver maxv_driver = {
	.driver = {
		.name		= "maxv",
		.of_match_table = maxv_of_match,
	},
	.probe		= maxv_probe,
	.remove		= maxv_remove,
};

module_spi_driver(maxv_driver);

MODULE_DESCRIPTION("Driver for maxv in a10 DK");
MODULE_AUTHOR("Qi ZHOU");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:maxv");
