# LSM9DS1 传感器底层驱动开发核心规范

## 1. 通信基础接口 (Communication Interface)
- **支持的物理层协议**: I2C 标准/快速模式 (100kHz & 400kHz), SPI (3-wire/4-wire).
- **I2C 通信参数**:
  - **Accel/Gyro (A/G) 器件地址**:
    - SA0 引脚接地 (0): 7-bit `0x6A` | 8-bit Write `0xD4` | 8-bit Read `0xD5`.
    - SA0 引脚接高 (1): 7-bit `0x6B` | 8-bit Write `0xD6` | 8-bit Read `0xD7`.
  - **Magnetometer (Mag) 器件地址**:
    - SA1 引脚接地 (0): 7-bit `0x1C` | 8-bit Write `0x38` | 8-bit Read `0x39`.
    - SA1 引脚接高 (1): 7-bit `0x1E` | 8-bit Write `0x3C` | 8-bit Read `0x3D`.
- **SPI 通信参数**:
  - 最大时钟频率: `10 MHz`.
  - 时钟极性与相位 (CPOL/CPHA): 空闲高电平 (SPC is stopped high)，下降沿数据变化，上升沿采样 (Mode 3).
  - 寄存器寻址: 首字节 Bit 0 为 R/W (0:Write, 1:Read)，多字节连续读写通过 `IF_ADD_INC` 控制地址自增.

## 2. 核心寄存器映射表 (Core Register Map)
```c
// --- 加速度计与陀螺仪 (A/G) 核心寄存器 ---
#define LSM9DS1_AG_WHO_AM_I_REG      0x0F // 0x68 : 设备识别码 (只读)
#define LSM9DS1_AG_CTRL_REG1_G_REG   0x10 // 0x00 : 陀螺仪控制1 (ODR, 满量程, 带宽)
#define LSM9DS1_AG_CTRL_REG4_REG     0x1E // 0x38 : 陀螺仪控制4 (轴使能)
#define LSM9DS1_AG_CTRL_REG5_XL_REG  0x1F // 0x38 : 加速度计控制5 (轴使能)
#define LSM9DS1_AG_CTRL_REG6_XL_REG  0x20 // 0x00 : 加速度计控制6 (ODR, 满量程, 带宽)
#define LSM9DS1_AG_CTRL_REG8_REG     0x22 // 0x04 : 控制寄存器8 (软复位, BDU, 地址自增)
#define LSM9DS1_AG_STATUS_REG        0x17 // 0x00 : A/G 数据就绪状态寄存器
#define LSM9DS1_AG_OUT_X_L_G_REG     0x18 // 陀螺仪 X 轴低字节起址
#define LSM9DS1_AG_OUT_X_L_XL_REG    0x28 // 加速度计 X 轴低字节起址

// --- 磁力计 (M) 核心寄存器 ---
#define LSM9DS1_M_WHO_AM_I_M_REG     0x0F // 0x3D : 磁力计识别码 (只读)
#define LSM9DS1_M_CTRL_REG1_M_REG    0x20 // 0x10 : 磁力计控制1 (温度补偿, XY工作模式, ODR)
#define LSM9DS1_M_CTRL_REG2_M_REG    0x21 // 0x00 : 磁力计控制2 (满量程, 软复位)
#define LSM9DS1_M_CTRL_REG3_M_REG    0x22 // 0x03 : 磁力计控制3 (转换模式)
#define LSM9DS1_M_STATUS_REG_M       0x27 // 磁力计数据就绪状态寄存器
#define LSM9DS1_M_OUT_X_L_M_REG      0x28 // 磁力计 X 轴低字节起址
```

## 3. 关键位域与掩码 (Key Bitfields & Masks)
```c
// === 通用控制位掩码 ===
#define LSM9DS1_AG_SW_RESET_MASK     0x01 // 触发 A/G 软件复位
#define LSM9DS1_AG_BDU_MASK          0x40 // Block Data Update (读取全16位前保护数据)
#define LSM9DS1_AG_IF_ADD_INC_MASK   0x04 // 允许 I2C/SPI 多字节读写时地址自增
#define LSM9DS1_M_SOFT_RST_MASK      0x04 // 触发磁力计软件复位

// === 陀螺仪量程 (CTRL_REG1_G, Bits [4:3]) ===
#define LSM9DS1_G_FS_245DPS          (0x00 << 3) // ±245 dps
#define LSM9DS1_G_FS_500DPS          (0x01 << 3) // ±500 dps
#define LSM9DS1_G_FS_2000DPS         (0x03 << 3) // ±2000 dps

// === 加速度计量程 (CTRL_REG6_XL, Bits [4:3]) ===
#define LSM9DS1_XL_FS_2G             (0x00 << 3) // ±2 g
#define LSM9DS1_XL_FS_16G            (0x01 << 3) // ±16 g
#define LSM9DS1_XL_FS_4G             (0x02 << 3) // ±4 g
#define LSM9DS1_XL_FS_8G             (0x03 << 3) // ±8 g

// === 磁力计量程 (CTRL_REG2_M, Bits [6:5]) ===
#define LSM9DS1_M_FS_4G              (0x00 << 5) // ±4 gauss
#define LSM9DS1_M_FS_8G              (0x01 << 5) // ±8 gauss
#define LSM9DS1_M_FS_12G             (0x02 << 5) // ±12 gauss
#define LSM9DS1_M_FS_16G             (0x03 << 5) // ±16 gauss

// === 磁力计工作模式 (CTRL_REG3_M, Bits [1:0]) ===
#define LSM9DS1_M_MD_CONTINUOUS      0x00 // 连续转换模式
#define LSM9DS1_M_MD_SINGLE          0x01 // 单次转换模式
#define LSM9DS1_M_MD_POWER_DOWN      0x02 // 掉电模式
```

## 4. 状态机与初始化流程 (Initialization Sequence)
1. **软件复位与总线配置 (Software Reset & Bus Setup)**
   - 写入 `CTRL_REG8` (`0x22`) = `0x05` (置位 `SW_RESET` 和 `IF_ADD_INC`)，触发 A/G 软复位并使能地址自增。
   - 写入 `CTRL_REG2_M` (`0x21`) = `0x0C` (置位 `SOFT_RST` 和 `REBOOT`)，触发磁力计复位。
   - *延时要求*: 至少等待 10ms - 20ms 以确保寄存器恢复默认状态。
   - 写入 `CTRL_REG8` (`0x22`) = `0x44` (置位 `BDU` 启用块数据更新，置位 `IF_ADD_INC`)。

2. **陀螺仪初始化 (Gyroscope Setup)**
   - 写入 `CTRL_REG1_G` (`0x10`) = 结合所需 ODR 和满量程配置。
   - 示例: `0x68` -> ODR = 119 Hz (`011`), 满量程 = ±500 dps (`01`), 带宽默认 (`00`)。

3. **加速度计初始化 (Accelerometer Setup)**
   - 写入 `CTRL_REG6_XL` (`0x20`) = 结合所需 ODR 和满量程配置。
   - 示例: `0x70` -> ODR = 119 Hz (`011`), 满量程 = ±4g (`10`), 带宽根据 ODR 自动适配 (`000`)。

4. **磁力计初始化 (Magnetometer Setup)**
   - 写入 `CTRL_REG1_M` (`0x20`) = `0x70` (启用温度补偿 `TEMP_COMP=1`, XY轴高性能模式 `OM=11`, ODR=20Hz `DO=101`)。
   - 写入 `CTRL_REG2_M` (`0x21`) = `0x00` (满量程 ±4 Gauss)。
   - 写入 `CTRL_REG3_M` (`0x22`) = `0x00` (配置为连续转换模式 `MD=00`)。

## 5. 传感参数控制、数据读取与转换逻辑

### 参数控制与传感模式
- **进入待机/掉电**: 将 `CTRL_REG1_G` 的 ODR 设置为 `000` 时，陀螺仪掉电。将 `CTRL_REG6_XL` 的 ODR 设为 `000`，加速度计掉电。磁力计向 `CTRL_REG3_M` 写入 `MD=10` 或 `11` 即可掉电。
- **块数据更新 (BDU)**: 强烈建议开启 (默认关闭)。在读取 MSB 之前，LSB 会被锁定，避免在两次 I2C/SPI 读取间隔中被新数据覆盖，产生错位数据。

### 数据读取判定 (Data Ready)
- **轮询读取 (Polling)**:
  - 读取 `STATUS_REG` (`0x17`): `Bit 0 (XLDA)` 为 1 表示加速度计就绪；`Bit 1 (GDA)` 为 1 表示陀螺仪就绪。
  - 读取 `STATUS_REG_M` (`0x27`): `Bit 3 (ZYXDA)` 为 1 表示磁力计 XYZ 轴数据就绪。
- **硬件中断触发 (Interrupt)**: 
  - 通过 `INT1_CTRL` (`0x0C`) 将 `INT_DRDY_G` 或 `INT_DRDY_XL` 映射到 `INT1_A/G` 引脚，响应外部中断进行读取。

### 数据拼接与解析 (Data Concatenation)
- 位深与大小端: 数据以 16-bit 输出，补码形式存在 (Two's complement)，默认使用小端模式 (Little Endian，低地址为低字节)。
- 拼接逻辑 (C代码范式):
  `int16_t raw_val = (int16_t)(((uint16_t)Reg_H << 8) | Reg_L);`

### 物理量转换公式与灵敏度系数
实际物理量 = `raw_val * Sensitivity` (注意转换为浮点运算并调整量级)

| 传感器 | 配置的量程 | 灵敏度系数 (Sensitivity) | 换算公式 (C代码逻辑) |
|---|---|---|---|
| **Accel** | ±2 g | 0.061 mg/LSB | `acc_g = raw_val * 0.061f / 1000.0f;` |
| **Accel** | ±4 g | 0.122 mg/LSB | `acc_g = raw_val * 0.122f / 1000.0f;` |
| **Accel** | ±8 g | 0.244 mg/LSB | `acc_g = raw_val * 0.244f / 1000.0f;` |
| **Accel** | ±16 g | 0.732 mg/LSB | `acc_g = raw_val * 0.732f / 1000.0f;` |
| **Gyro** | ±245 dps | 8.75 mdps/LSB | `gyro_dps = raw_val * 8.75f / 1000.0f;` |
| **Gyro** | ±500 dps | 17.50 mdps/LSB | `gyro_dps = raw_val * 17.50f / 1000.0f;` |
| **Gyro** | ±2000 dps | 70 mdps/LSB | `gyro_dps = raw_val * 70.0f / 1000.0f;` |
| **Mag** | ±4 gauss | 0.14 mgauss/LSB | `mag_gauss = raw_val * 0.14f / 1000.0f;` |
| **Mag** | ±8 gauss | 0.29 mgauss/LSB | `mag_gauss = raw_val * 0.29f / 1000.0f;` |
| **Mag** | ±12 gauss | 0.43 mgauss/LSB | `mag_gauss = raw_val * 0.43f / 1000.0f;` |
| **Mag** | ±16 gauss | 0.58 mgauss/LSB | `mag_gauss = raw_val * 0.58f / 1000.0f;` |