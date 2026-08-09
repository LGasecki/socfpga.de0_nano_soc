#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/sysfs.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/of.h> 

#define ADXL345_REG_POWER_CTL   0x2D
#define ADXL345_REG_DATA_FORMAT 0x31
#define ADXL345_REG_BW_RATE     0x2C
#define ADXL345_REG_DATAX0      0x32

struct adxl345_data {
    struct i2c_client *client;
    bool is_measuring;
};

// =====================================================================
// Helper function: Common I2C data read for all attributes
// =====================================================================
static int read_xyz_data(struct adxl345_data *adxl_data, s16 *x, s16 *y, s16 *z) {
    u8 data[6];

    if (!adxl_data->is_measuring) {
        return -1; // Error code: Sensor is in standby mode
    }

    if (i2c_smbus_read_i2c_block_data(adxl_data->client, ADXL345_REG_DATAX0, 6, data) < 0) {
        return -2; // Error code: I2C bus error
    }

    *x = (data[1] << 8) | data[0];
    *y = (data[3] << 8) | data[2];
    *z = (data[5] << 8) | data[4];

    return 0; // Success
}

// =====================================================================
// Attribute: xyz 
// =====================================================================
ssize_t xyz_show(struct device *dev, struct device_attribute *attr, char *buf) {
    s16 x, y, z;
    int status = read_xyz_data(dev_get_drvdata(dev), &x, &y, &z);
    
    if (status == -1) return sprintf(buf, "Sensor is in standby mode (STANDBY).\n");
    if (status == -2) return sprintf(buf, "I2C communication error.\n");

    return sprintf(buf, "X: %d mg, Y: %d mg, Z: %d mg\n", x * 4, y * 4, z * 4);
};

struct device_attribute xyz_attribute = {
    .attr = {
        .name = "xyz",
        .mode = 0444, 
    },
    .show = xyz_show,
};

// =====================================================================
// Individual axis attributes 
// =====================================================================

// X-axis
ssize_t x_show(struct device *dev, struct device_attribute *attr, char *buf) {
    s16 x, y, z;
    if (read_xyz_data(dev_get_drvdata(dev), &x, &y, &z) < 0) 
		return -ENODATA;
    return sprintf(buf, "%d\n", x * 4);
};
struct device_attribute x_attribute = {
    .attr = { 
		.name = "x", 
		.mode = 0444 
		},
    .show = x_show,
};

// Y-axis
ssize_t y_show(struct device *dev, struct device_attribute *attr, char *buf) {
    s16 x, y, z;
    if (read_xyz_data(dev_get_drvdata(dev), &x, &y, &z) < 0) 
		return -ENODATA;
    return sprintf(buf, "%d\n", y * 4);
};
struct device_attribute y_attribute = {
    .attr = { 
		.name = "y", 
		.mode = 0444 
		},
    .show = y_show,
};

// Z-axis
ssize_t z_show(struct device *dev, struct device_attribute *attr, char *buf) {
    s16 x, y, z;
    if (read_xyz_data(dev_get_drvdata(dev), &x, &y, &z) < 0) 
		return -ENODATA;
    return sprintf(buf, "%d\n", z * 4);
};
struct device_attribute z_attribute = {
    .attr = { 
		.name = "z", 
		.mode = 0444 
	},
    .show = z_show,
};

// =====================================================================
// Attribute: power_mode 
// =====================================================================
ssize_t power_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    struct adxl345_data *adxl_data = dev_get_drvdata(dev);
    unsigned int mode;

    if (kstrtouint(buf, 10, &mode) < 0) {
        return -EINVAL;
    }

    if (mode == 1) {
        i2c_smbus_write_byte_data(adxl_data->client, ADXL345_REG_POWER_CTL, 0x08); 
        adxl_data->is_measuring = true;
    } else if (mode == 0) {
        i2c_smbus_write_byte_data(adxl_data->client, ADXL345_REG_POWER_CTL, 0x00); 
        adxl_data->is_measuring = false;
    }

    return count;
};

struct device_attribute power_mode_attribute = {
    .attr = {
        .name = "power_mode",
        .mode = 0222,
    },
    .store = power_mode_store,
};

// =====================================================================
// PROBE 
// =====================================================================
int probe(struct i2c_client *client, const struct i2c_device_id *id) {
    struct adxl345_data *adxl_data = devm_kmalloc(&client->dev, sizeof(struct adxl345_data), GFP_KERNEL);
    if (!adxl_data) {
        return -ENOMEM;
    }

    adxl_data->client = client;
    
    // Start in standby mode by default (power saving)
    adxl_data->is_measuring = false; 
    dev_set_drvdata(&client->dev, adxl_data);

    i2c_smbus_write_byte_data(client, ADXL345_REG_DATA_FORMAT, 0x08); 
    i2c_smbus_write_byte_data(client, ADXL345_REG_BW_RATE, 0x09);     
    i2c_smbus_write_byte_data(client, ADXL345_REG_POWER_CTL, 0x00); // Set to standby  

    // Register all sysfs files
    device_create_file(&client->dev, &xyz_attribute);
    device_create_file(&client->dev, &x_attribute);
    device_create_file(&client->dev, &y_attribute);
    device_create_file(&client->dev, &z_attribute);
    device_create_file(&client->dev, &power_mode_attribute);

    pr_info("ADXL345: Initialization successful.\n");
    return 0;
}

// =====================================================================
// REMOVE
// =====================================================================
void remove(struct i2c_client *client) {
    struct adxl345_data *adxl_data = dev_get_drvdata(&client->dev);

    // Remove all sysfs files
    device_remove_file(&client->dev, &xyz_attribute);
    device_remove_file(&client->dev, &x_attribute);
    device_remove_file(&client->dev, &y_attribute);
    device_remove_file(&client->dev, &z_attribute);
    device_remove_file(&client->dev, &power_mode_attribute);
    
    i2c_smbus_write_byte_data(adxl_data->client, ADXL345_REG_POWER_CTL, 0x00);

    pr_info("ADXL345: Driver uninstalled.\n");
}

// =====================================================================
// Match tables and driver registration
// =====================================================================

static const struct of_device_id adxl345_of_match[] = {
    { .compatible = "asicsagh,adxl345" },
    { }
};
MODULE_DEVICE_TABLE(of, adxl345_of_match);

static const struct i2c_device_id match_table[] = {
    { "adxl345", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, match_table);

struct i2c_driver my_driver = {
    .probe = probe,
    .remove = remove,
    .driver = {
        .name = "my_driver",
        .of_match_table = adxl345_of_match, 
    },
    .id_table = match_table,
};

module_i2c_driver(my_driver);

MODULE_AUTHOR("Lukasz Gasecki");
MODULE_LICENSE("GPL");