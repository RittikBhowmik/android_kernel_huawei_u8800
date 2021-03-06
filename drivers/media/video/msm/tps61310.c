/* <DTS2012041003722 sibingsong 20120410 begin */
/*< DTS2011092805375   yuguangcai 20110928 begin */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <asm/gpio.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <mach/camera.h>
#include <asm/mach-types.h>
/* < DTS2012022004193 yangyang 20120220 begin */ 
#include <linux/hardware_self_adapt.h>
#include <linux/gpio.h>
#include <mach/pmic.h>
#include <linux/gpio.h>
#include <linux/mfd/pmic8058.h>

//#define tps61310_nreset	27
/* DTS2012022004193 yangyang 20120220 end >*/
#define tps61310_strb1	125
#define tps61310_strb0	117
/*< DTS2011122006722   songxiaoming 20120207 begin */
#define PMIC_GPIO_FLASH_EN 22    /*  PMIC GPIO NUMBER 23   */
#define PMIC_GPIO_TORCH_FLASH 23    /*  PMIC GPIO NUMBER 24   */
#define FLASH_REFRESHED_TIME 10 /*10s*/
static struct hrtimer flash_timer;
static struct work_struct flash_work;
static bool timer_is_run = false;
/* DTS2011122006722  songxiaoming 20120207 end > */
/* < DTS2012022004193 yangyang 20120220 begin */ 
static unsigned tps61310_nreset = 0;
/* DTS2012022004193 yangyang 20120220 end >*/
//if use flash mode
//#define FLASH_MODE
//if use video mode  

struct i2c_client *tps61310_client;

static int tps61310_i2c_write(struct i2c_client *client,
				    uint8_t reg,uint8_t val)
{
	int ret;

	if (tps61310_client == NULL){ 
		printk("tps61310_client is NULL!\n");
		return -1;
	}
	
	ret = i2c_smbus_write_byte_data(tps61310_client,reg,val);
	if (ret < 0) {
		printk("tps61310_i2c_write write error\n");
	}

	return ret;
}

static int tps61310_i2c_read(struct i2c_client *client,uint8_t reg)
{
	int val = 0;
	if (tps61310_client == NULL){  /*  No global client pointer? */
		printk("tps61310_client is NULL!\n");
		return -1;
	}

	val = i2c_smbus_read_byte_data(tps61310_client, reg);
	if (val >= 0) {
		printk(" tps61310_i2c_read : reg[%x] = %x\n ",reg,val);
	} else {
		printk(" tps61310_i2c_read error\n ");
	}
		
	return val;
}
/*< DTS2011122006722   songxiaoming 20120207 begin */
static enum hrtimer_restart flash_timer_func(struct hrtimer *timer)
{
    schedule_work(&flash_work);
    return HRTIMER_NORESTART;
}

/* To avoid device shutdown when timeout, 
 * 0x01 and 0x02 bits need to be refreshed within less than 13.0s
 * use timer to do it
 */
static void flash_on(struct work_struct *work)
{
    printk("flash_on!\n");
    tps61310_i2c_write(tps61310_client, 0x01, 0x40 );
    tps61310_i2c_write(tps61310_client, 0x02, 0x40 );

    /* Restart timer ,so it can be recurrent */
    hrtimer_start(&flash_timer, ktime_set(FLASH_REFRESHED_TIME, 0), HRTIMER_MODE_REL);
}
/* DTS2011122006722  songxiaoming 20120207 end > */

/*different handlings of different flash's states*/
int tps61310_set_flash(unsigned led_state)
{
    int rc = 0;

    printk("tps61310_set_flash: led_state = %d\n", led_state);
    /*< DTS2011122006722   songxiaoming 20120207 begin */
    /* timer should be cancel,when Led_state possible changed */
    if(timer_is_run)
    {
        hrtimer_cancel(&flash_timer);
        timer_is_run = false;        
    }
    /* DTS2011111606482 zhouqiwei 20111215 end > */

    switch (led_state)
    {
    case MSM_CAMERA_LED_LOW:
#ifdef CONFIG_ARCH_MSM7X30        
        pm8xxx_gpio_set_value(PMIC_GPIO_FLASH_EN, 1);
        pm8xxx_gpio_set_value(PMIC_GPIO_TORCH_FLASH, 1);
        mdelay(1);
#endif        
        tps61310_i2c_write( tps61310_client,0x00, 0x0A );
        tps61310_i2c_write( tps61310_client,0x05, 0x6F );
        tps61310_i2c_write( tps61310_client,0x01, 0x40 );
        tps61310_i2c_write( tps61310_client,0x02, 0x40 );
        INIT_WORK(&flash_work, flash_on);
        hrtimer_init(&flash_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        flash_timer.function= flash_timer_func;
        timer_is_run = true;
        hrtimer_start(&flash_timer, ktime_set(FLASH_REFRESHED_TIME, 0), HRTIMER_MODE_REL);


        break;

    case MSM_CAMERA_LED_HIGH:
        /* < DTS2012030604199 tangying 20120306 begin */
#ifdef CONFIG_ARCH_MSM7X27A        
        gpio_set_value(tps61310_strb0, 1);
#else
        pm8xxx_gpio_set_value(PMIC_GPIO_FLASH_EN, 1);
        pm8xxx_gpio_set_value(PMIC_GPIO_TORCH_FLASH, 1);
        mdelay(1);
#endif
        tps61310_i2c_write( tps61310_client, 0x03, 0xE7 );
        tps61310_i2c_write( tps61310_client, 0x05, 0x6F );
        tps61310_i2c_write( tps61310_client, 0x01, 0x88 );
        tps61310_i2c_write( tps61310_client, 0x02, 0x88 );
        /* DTS2012030604199 tangying 20120306 end > */

        break;


    case MSM_CAMERA_LED_TORCH:
#ifdef CONFIG_ARCH_MSM7X30        
        pm8xxx_gpio_set_value(PMIC_GPIO_FLASH_EN, 1);
        pm8xxx_gpio_set_value(PMIC_GPIO_TORCH_FLASH, 1);
        mdelay(1);
#endif        
        tps61310_i2c_write( tps61310_client,0x00, 0x0A );
        tps61310_i2c_write( tps61310_client,0x05, 0x6F );
        tps61310_i2c_write( tps61310_client,0x01, 0x40 );
        tps61310_i2c_write( tps61310_client,0x02, 0x40 );
        INIT_WORK(&flash_work, flash_on);
        hrtimer_init(&flash_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        flash_timer.function= flash_timer_func;
        timer_is_run = true;
        hrtimer_start(&flash_timer, ktime_set(FLASH_REFRESHED_TIME, 0), HRTIMER_MODE_REL);
        break;
    /* DTS2011122006722  songxiaoming 20120207 end > */

    case MSM_CAMERA_LED_OFF:
        tps61310_i2c_write( tps61310_client, 0x00, 0x80 );
        /* < DTS2012030604199 tangying 20120306 begin */
#ifdef CONFIG_ARCH_MSM7X27A                
        gpio_set_value(tps61310_strb0, 0);
#else
        /* DTS2012030604199 tangying 20120306 end > */
        mdelay(1);
        pm8xxx_gpio_set_value(PMIC_GPIO_FLASH_EN, 0);
        pm8xxx_gpio_set_value(PMIC_GPIO_TORCH_FLASH, 0);
#endif        
        break;

    default:
        tps61310_i2c_write( tps61310_client, 0x00, 0x80 );
        /* < DTS2012030604199 tangying 20120306 begin */
#ifdef CONFIG_ARCH_MSM7X27A                
        gpio_set_value(tps61310_strb0, 0);
#else
        /* DTS2012030604199 tangying 20120306 end > */
        mdelay(1);
        pm8xxx_gpio_set_value(PMIC_GPIO_FLASH_EN, 0);
        pm8xxx_gpio_set_value(PMIC_GPIO_TORCH_FLASH, 0);
#endif        
        break;
    }

    return rc;
}

static int tps61310_device_init(void)
{	
	
	int err = 0;
	int gpio_config;
	/* < DTS2012030604199 tangying 20120306 begin */
	err = gpio_request(tps61310_strb0, "tps61310_strb0");
	if(err)
	{
		printk(KERN_ERR "%s: gpio_requset gpio %d failed, err=%d!\n", __func__, tps61310_strb0, err);
	}

	gpio_config = GPIO_CFG(tps61310_strb0, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA);
	err = gpio_tlmm_config(gpio_config, GPIO_CFG_ENABLE);
	if(err)
	{
		printk(KERN_ERR "%s: gpio_tlmm_config(#%d)=%d\n", __func__, tps61310_strb0, err);
	}
	/* DTS2012030604199 tangying 20120306 end > */

	/*config the tps61310_nreset gpio to output state*/
	gpio_config = GPIO_CFG(tps61310_nreset, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA);
	/*enable the above config*/
	err = gpio_tlmm_config(gpio_config, GPIO_CFG_ENABLE);
	if (err) 
	{
		err = -EIO;
		printk(KERN_ERR "%s: gpio_tlmm_config(#%d)=%d\n", __func__, tps61310_nreset, err);
	}	
	/*request the gpio having been config previouly */
	err = gpio_request_one(tps61310_nreset,GPIOF_OUT_INIT_HIGH,"tps61310_nreset");
	if(err)
		printk("tps61310 gpio request: tps61310_nreset failed!\n");
				
	return err;
}

static int tps61310_probe(struct i2c_client *client,
			       const struct i2c_device_id *devid)
{
	int ret = -1;
	int tempvalue = 0;
	
	printk("tps61310_probe start!\n");

	tps61310_client = client;
	/* DTS2012022004193 yangyang 20120220 end >*/
	/* GPIO 27 used by BT of some products, we use GPIO 83 instead. */
	if(HW_BT_WAKEUP_GPIO_IS_27 == get_hw_bt_wakeup_gpio_type())
	{
		tps61310_nreset = 83;
	}
	else
	{
		tps61310_nreset = 27;
	}
	/* DTS2012022004193 yangyang 20120220 end >*/
	/*device init of gpio init*/
#ifdef CONFIG_ARCH_MSM7X27A    
	if ( tps61310_device_init()){
		printk("tps61310_device_init error!\n");	
	}
#endif	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		ret = -ENODEV;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK)){
		ret = -ENODEV;
	}
	
	/* read chip id */
	tempvalue = tps61310_i2c_read(tps61310_client, 0x07);
	if ((tempvalue & 0x07) == 0x06) {
		printk("tps61310 read chip id ok!\n");
	} else {
		printk("tps61310 read chip id error!\n");
	}
	
	printk("tps61310_probe end!\n");
	return 0;
}

static int tps61310_remove(struct i2c_client *client)
{
	printk("tps61310 driver removing\n");
	gpio_free(tps61310_nreset);
	return 0;
}

static const struct i2c_device_id tps61310_id[] = {
	{ "tps61310", 0 },
	{},
};

MODULE_DEVICE_TABLE(i2c, tps61310_id);

static struct i2c_driver tps61310_driver = {
	.probe = tps61310_probe,
	.remove = __devexit_p(tps61310_remove),
	.id_table = tps61310_id,
	.driver = {
	.owner = THIS_MODULE,
	.name = "tps61310",
	},
};

static int __init tps61310_init(void)
{
	printk(KERN_INFO "tps61310 init driver\n");
	return i2c_add_driver(&tps61310_driver);
}

static void __exit tps61310_exit(void)
{
	printk(KERN_INFO "tps61310 exit\n");
	i2c_del_driver(&tps61310_driver);
	return;
}

module_init(tps61310_init);
module_exit(tps61310_exit);

MODULE_DESCRIPTION("tps61310 light driver");
MODULE_LICENSE("GPL");
/* DTS2011092805375   yuguangcai 20110928 end > */
/* DTS2012041003722 sibingsong 20120410 end> */
