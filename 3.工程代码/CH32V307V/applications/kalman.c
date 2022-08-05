/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-08-01     Misth       the first version
 */
#include "kalman.h"

#define RAD_TO_DEG 57.295779513082320876798154814105
#define MPU6XXX_DEVICE_NAME  "i2c1"

uint32_t timer;

Kalman_t KalmanX = {
    .Q_angle = 0.001f,
    .Q_bias = 0.003f,
    .R_measure = 0.03f};

Kalman_t KalmanY = {
    .Q_angle = 0.001f,
    .Q_bias = 0.003f,
    .R_measure = 0.03f,
};

void MPU6050_Read_All(struct IMU_Parameter *IMU_Data)
{
    double dt = (double)(rt_tick_get() - timer) / 1000;
    timer = rt_tick_get();
    double roll;
    double roll_sqrt = sqrt(IMU_Data->Accel_X * IMU_Data->Accel_X + IMU_Data->Accel_Z * IMU_Data->Accel_Z);
    if (roll_sqrt != 0.0)
    {
        roll = atan(IMU_Data->Accel_Y / roll_sqrt) * RAD_TO_DEG;
    }
    else
    {
        roll = 0.0;
    }
    double pitch = atan2(-IMU_Data->Accel_X, IMU_Data->Accel_Z) * RAD_TO_DEG;
    if ((pitch < -90 && IMU_Data->KalmanAngleY > 90) || (pitch > 90 && IMU_Data->KalmanAngleY < -90))
    {
        KalmanY.angle = pitch;
        IMU_Data->KalmanAngleY = pitch;
    }
    else
    {
        IMU_Data->KalmanAngleY = Kalman_getAngle(&KalmanY, pitch, IMU_Data->Gyro_Y, dt);
    }
    if (fabs(IMU_Data->KalmanAngleY) > 90)
        IMU_Data->Gyro_X = -IMU_Data->Gyro_X;
    IMU_Data->KalmanAngleX = Kalman_getAngle(&KalmanX, roll, IMU_Data->Gyro_Y, dt);
}

double Kalman_getAngle(Kalman_t *Kalman, double newAngle, double newRate, double dt)
{
    double rate = newRate - Kalman->bias;
    Kalman->angle += dt * rate;

    Kalman->P[0][0] += dt * (dt * Kalman->P[1][1] - Kalman->P[0][1] - Kalman->P[1][0] + Kalman->Q_angle);
    Kalman->P[0][1] -= dt * Kalman->P[1][1];
    Kalman->P[1][0] -= dt * Kalman->P[1][1];
    Kalman->P[1][1] += Kalman->Q_bias * dt;

    double S = Kalman->P[0][0] + Kalman->R_measure;
    double K[2];
    K[0] = Kalman->P[0][0] / S;
    K[1] = Kalman->P[1][0] / S;

    double y = newAngle - Kalman->angle;
    Kalman->angle += K[0] * y;
    Kalman->bias += K[1] * y;

    double P00_temp = Kalman->P[0][0];
    double P01_temp = Kalman->P[0][1];

    Kalman->P[0][0] -= K[0] * P00_temp;
    Kalman->P[0][1] -= K[0] * P01_temp;
    Kalman->P[1][0] -= K[1] * P00_temp;
    Kalman->P[1][1] -= K[1] * P01_temp;

    return Kalman->angle;
}




void Kalman_get_value(struct mpu6xxx_device *dev, struct IMU_Parameter *IMU_Data)
{
    struct mpu6xxx_3axes accel, gyro;
    mpu6xxx_get_accel(dev, &accel);
    mpu6xxx_get_gyro(dev, &gyro);

    IMU_Data->Accel_X = accel.x;
    IMU_Data->Accel_Y = accel.y;
    IMU_Data->Accel_Z = accel.z;
    IMU_Data->Gyro_X = gyro.x;
    IMU_Data->Gyro_Y = gyro.y;
    IMU_Data->Gyro_Z = gyro.z;

    MPU6050_Read_All(IMU_Data);
}
