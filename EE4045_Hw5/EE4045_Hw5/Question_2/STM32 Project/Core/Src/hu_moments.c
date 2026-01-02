/*
 * hu_moments.c
 *
 *  Created on: Jan 2, 2026
 *      Author: muham
 */


#include "hu_moments.h"
#include <math.h>

void hu_moments_7_from_u8_28x28(const uint8_t img[28][28], float hu7[7])
{
    double m00=0, m10=0, m01=0, m11=0, m20=0, m02=0, m30=0, m03=0, m12=0, m21=0;

    for (int y=0; y<28; y++) {
        for (int x=0; x<28; x++) {
            // binaryImage=True mantığı: piksel >0 ise 1 kabul
            double I = (img[y][x] > 0) ? 1.0 : 0.0;
            if (I == 0.0) continue;

            double xd = (double)x;
            double yd = (double)y;

            m00 += I;
            m10 += xd * I;
            m01 += yd * I;
            m11 += xd * yd * I;
            m20 += xd * xd * I;
            m02 += yd * yd * I;
            m30 += xd * xd * xd * I;
            m03 += yd * yd * yd * I;
            m12 += xd * yd * yd * I;
            m21 += xd * xd * yd * I;
        }
    }

    if (m00 <= 0.0) {
        for (int i=0;i<7;i++) hu7[i]=0.0f;
        return;
    }

    double xbar = m10 / m00;
    double ybar = m01 / m00;

    double mu11=0, mu20=0, mu02=0, mu30=0, mu03=0, mu12=0, mu21=0;

    for (int y=0; y<28; y++) {
        for (int x=0; x<28; x++) {
            double I = (img[y][x] > 0) ? 1.0 : 0.0;
            if (I == 0.0) continue;

            double dx = (double)x - xbar;
            double dy = (double)y - ybar;

            mu11 += dx * dy * I;
            mu20 += dx * dx * I;
            mu02 += dy * dy * I;
            mu30 += dx * dx * dx * I;
            mu03 += dy * dy * dy * I;
            mu12 += dx * dy * dy * I;
            mu21 += dx * dx * dy * I;
        }
    }

    double mu00 = m00;
    double mu00_2_0 = mu00 * mu00;       // p+q=2
    double mu00_2_5 = pow(mu00, 2.5);    // p+q=3

    double eta11 = mu11 / mu00_2_0;
    double eta20 = mu20 / mu00_2_0;
    double eta02 = mu02 / mu00_2_0;

    double eta30 = mu30 / mu00_2_5;
    double eta03 = mu03 / mu00_2_5;
    double eta12 = mu12 / mu00_2_5;
    double eta21 = mu21 / mu00_2_5;

    double h1 = eta20 + eta02;
    double h2 = (eta20 - eta02)*(eta20 - eta02) + 4.0*eta11*eta11;
    double h3 = (eta30 - 3.0*eta12)*(eta30 - 3.0*eta12) + (3.0*eta21 - eta03)*(3.0*eta21 - eta03);
    double h4 = (eta30 + eta12)*(eta30 + eta12) + (eta21 + eta03)*(eta21 + eta03);
    double h5 = (eta30 - 3.0*eta12)*(eta30 + eta12)*((eta30 + eta12)*(eta30 + eta12) - 3.0*(eta21 + eta03)*(eta21 + eta03))
              + (3.0*eta21 - eta03)*(eta21 + eta03)*(3.0*(eta30 + eta12)*(eta30 + eta12) - (eta21 + eta03)*(eta21 + eta03));
    double h6 = (eta20 - eta02)*((eta30 + eta12)*(eta30 + eta12) - (eta21 + eta03)*(eta21 + eta03))
              + 4.0*eta11*(eta30 + eta12)*(eta21 + eta03);
    double h7 = (3.0*eta21 - eta03)*(eta30 + eta12)*((eta30 + eta12)*(eta30 + eta12) - 3.0*(eta21 + eta03)*(eta21 + eta03))
              - (eta30 - 3.0*eta12)*(eta21 + eta03)*(3.0*(eta30 + eta12)*(eta30 + eta12) - (eta21 + eta03)*(eta21 + eta03));

    hu7[0] = (float)h1;
    hu7[1] = (float)h2;
    hu7[2] = (float)h3;
    hu7[3] = (float)h4;
    hu7[4] = (float)h5;
    hu7[5] = (float)h6;
    hu7[6] = (float)h7;
}
