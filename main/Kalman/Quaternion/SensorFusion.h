// Copyright (c) 2014, Freescale Semiconductor, Inc.
// All rights reserved.
// vim: set ts=4:
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Freescale Semiconductor, Inc. nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL FREESCALE SEMICONDUCTOR, INC. BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Adapted for this project; upstream: PaulStoffregen/NXPMotionSense.

#ifndef SENSORFUSION_H
#define SENSORFUSION_H

//https://github.com/PaulStoffregen/NXPMotionSense/blob/master/SensorFusion.cpp
//Ich küss dini Auge Paul Stoffregen

#include <inttypes.h>

#define FQVA_9DOF_GBY_KALMAN 2E-6F              // accelerometer noise g^2 so 1.4mg RMS
#define FQVM_9DOF_GBY_KALMAN 0.1F               // magnetometer noise uT^2
#define FQVG_9DOF_GBY_KALMAN 0.3F               // gyro noise (deg/s)^2
#define FQWB_9DOF_GBY_KALMAN 1E-9F              // gyro offset drift (deg/s)^2: 1E-9 implies 0.09deg/s max at 50Hz
#define FQWA_9DOF_GBY_KALMAN 1E-4F              // linear acceleration drift g^2 (increase slows convergence to g but reduces sensitivity to shake)
#define FQWD_9DOF_GBY_KALMAN 0.5F               // magnetic disturbance drift uT^2 (increase slows convergence to B but reduces sensitivity to magnet)
// initialization of Qw covariance matrix
#define FQWINITTHTH_9DOF_GBY_KALMAN 2000E-5F    // th_e * th_e terms
#define FQWINITBB_9DOF_GBY_KALMAN 250E-3F       // b_e * b_e terms
#define FQWINITTHB_9DOF_GBY_KALMAN 0.0F         // th_e * b_e terms
#define FQWINITAA_9DOF_GBY_KALMAN 10E-5F        // a_e * a_e terms (increase slows convergence to g but reduces sensitivity to shake)
#define FQWINITDD_9DOF_GBY_KALMAN 600E-3F       // d_e * d_e terms (increase slows convergence to B but reduces sensitivity to magnet)
// linear acceleration and magnetic disturbance time constants
#define FCA_9DOF_GBY_KALMAN 0.5F                // linear acceleration decay factor
#define FCD_9DOF_GBY_KALMAN 0.5F                // magnetic disturbance decay factor
// maximum geomagnetic inclination angle tracked by Kalman filter
#define SINDELTAMAX 0.9063078F      // sin of max +ve geomagnetic inclination angle: here 65.0 deg
#define COSDELTAMAX 0.4226183F      // cos of max +ve geomagnetic inclination angle: here 65.0 deg
#define DEFAULTB 50.0F              // default geomagnetic field (uT)
#define X 0                         // vector components
#define Y 1
#define Z 2
#define FDEGTORAD 0.01745329251994F		// degrees to radians conversion = pi / 180
#define FRADTODEG 57.2957795130823F		// radians to degrees conversion = 180 / pi
#define ONEOVER48 0.02083333333F		// 1 / 48
#define ONEOVER3840 0.0002604166667F	// 1 / 3840

typedef struct Quaternion_t{
    float q0; // w
	float q1; // x
	float q2; // y
	float q3; // z
} Quaternion_t;

typedef struct kalmanQuaternion_t{
    float PhiPl;                    // roll (deg)
    float ThePl;                    // pitch (deg)
    float PsiPl;                    // yaw (deg)
    float RhoPl;                    // compass (deg)
    float ChiPl;                    // tilt from vertical (deg)
    // orientation matrix, quaternion and rotation vector
    float RPl[3][3];                // a posteriori orientation matrix
    Quaternion_t qPl;               // a posteriori orientation quaternion
    float RVecPl[3];                // rotation vector
    // angular velocity
    float Omega[3];                 // angular velocity (deg/s)
    // systick timer for benchmarking
    int32_t systick;                // systick timer;
    // end: elements common to all motion state vectors

    // elements transmitted over bluetooth in kalman packet
    float bPl[3];                   // gyro offset (deg/s)
    float ThErrPl[3];               // orientation error (deg)
    float bErrPl[3];                // gyro offset error (deg/s)
    // end elements transmitted in kalman packet

    float dErrGlPl[3];              // magnetic disturbance error (uT, global frame)
    float dErrSePl[3];              // magnetic disturbance error (uT, sensor frame)
    float aErrSePl[3];              // linear acceleration error (g, sensor frame)
    float aSeMi[3];                 // linear acceleration (g, sensor frame)
    float DeltaPl;                  // inclination angle (deg)
    float aSePl[3];                 // linear acceleration (g, sensor frame)
    float aGlPl[3];                 // linear acceleration (g, global frame)
    float gErrSeMi[3];              // difference (g, sensor frame) of gravity vector (accel) and gravity vector (gyro)
    float mErrSeMi[3];              // difference (uT, sensor frame) of geomagnetic vector (magnetometer) and geomagnetic vector (gyro)
    float gSeGyMi[3];               // gravity vector (g, sensor frame) measurement from gyro
    float mSeGyMi[3];               // geomagnetic vector (uT, sensor frame) measurement from gyro
    float mGl[3];                   // geomagnetic vector (uT, global frame)
    float QvAA;                     // accelerometer terms of Qv
    float QvMM;                     // magnetometer terms of Qv
    float PPlus12x12[12][12];       // covariance matrix P+
    float K12x6[12][6];             // kalman filter gain matrix K
    float Qw12x12[12][12];          // covariance matrix Qw
    float C6x12[6][12];             // measurement matrix C
    float RMi[3][3];                // a priori orientation matrix
    Quaternion_t Deltaq;            // delta quaternion
    Quaternion_t qMi;               // a priori orientation quaternion
    float casq;                     // FCA * FCA;
    float cdsq;                     // FCD * FCD;
    float Fastdeltat;               // sensor sampling interval (s) = 1 / SENSORFS
    float deltat;                   // kalman filter sampling interval (s) = OVERSAMPLE_RATIO / SENSORFS
    float deltatsq;                 // fdeltat * fdeltat
    float QwbplusQvG;               // FQWB + FQVG
    int8_t FirstOrientationLock;    // denotes that 9DOF orientation has locked to 6DOF
    int8_t resetflag;               // flag to request re-initialization on next pass
} kalmanQuaternion_t;

void kalmanQuaternionInit(float sampleRate, kalmanQuaternion_t *kalmanState);
void kalmanQuaternionUpdate(kalmanQuaternion_t *kalmanState, float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz);

#endif