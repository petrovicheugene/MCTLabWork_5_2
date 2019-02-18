//=====================================================
#include <omp.h>
#include <iostream>
#include <math.h>
#include <random>
#include <ctime>
//=====================================================
using namespace std;
//=====================================================
struct Point4D
{
    Point4D(double x = 0.0, double y = 0.0, double z = 0.0, double f = 0.0)
    {
        this->x = x;
        this->y = y;
        this->z = z;
        this->f = f;
    }

    double x;
    double y;
    double z;
    double f;

};
//=====================================================
class RandomPoint4DGenerator
{
public:
    RandomPoint4DGenerator(int seedMultiplier, double xMin, double xMax,
                           double yMin, double yMax,
                           double zMin, double zMax,
                           double fMin, double fMax)
    {
        genX = mt19937 (static_cast<unsigned int>(seedMultiplier * time(nullptr)));
        distX = uniform_real_distribution<> (xMin, xMax);

        genY = mt19937 (static_cast<unsigned int>(123 *seedMultiplier * time(nullptr)));
        distY = uniform_real_distribution<> (yMin, yMax);


        genZ = mt19937 (static_cast<unsigned int>(456 *seedMultiplier * time(nullptr)));
        distZ = uniform_real_distribution<> (zMin, zMax);

        genF = mt19937 (static_cast<unsigned int>(789 *seedMultiplier * time(nullptr)));
        distF = uniform_real_distribution<> (fMin, fMax);

    }

    void generate(Point4D& point)
    {
        point.x = distX(genX);
        point.y = distY(genY);
        point.z = distZ(genZ);
        point.f = distF(genF);

    }

private:

    mt19937 genX;
    uniform_real_distribution<> distX;
    mt19937 genY;
    uniform_real_distribution<> distY;
    mt19937 genZ;
    uniform_real_distribution<> distZ;
    mt19937 genF;
    uniform_real_distribution<> distF;

};
//=====================================================


static double XMin = 0.0;
static double XMax = 0.0;
static double YMin = 0.0;
static double YMax = 0.0;
static double ZMin = 0.0;
static double ZMax = 0.0;

static double FMin = 0.0;
static double FMax = 0.0;

static long N = 0;
static double integral = 0.0;

inline double f(double x, double y, double z);
inline double f(Point4D point);

bool defineFMinMax();
bool inputInitData();
void calculateIntegral();
void outputResult();

//=====================================================
int main()
{
    cout << "Evaluation of multiple integral using Monte Carlo method." << endl;
    cout << "Function: pow(x, 2.0) * sin(x*y) * exp(-z*y)" << endl << endl;

    if(inputInitData() && defineFMinMax())
    {
        calculateIntegral();
        outputResult();
    }

    system("pause");
    return 0;
}
//=====================================================
double f(double x, double y, double z)
{
    return pow(x, 2.0) * sin(x*y) * exp(-z*y);
}
//=====================================================
double f(Point4D point)
{
    return f(point.x, point.y, point.z);
}
//=====================================================
bool defineFMinMax()
{
    int intervalCount = 100;
    double dx = (XMax - XMin) / static_cast<double>(intervalCount);
    double dy = (YMax - YMin) / static_cast<double>(intervalCount);
    double dz = (ZMax - ZMin) / static_cast<double>(intervalCount);

    FMin = f(XMin, YMin, ZMin);
    FMin = FMax;

#pragma omp parallel
    {
        double x;
        double y;
        double z;

        double fVal;
        double localMin = FMin;
        double localMax = FMax;
#pragma omp for
        for(int ix = 0; ix < intervalCount; ix++)
        {
            for(int iy = 0; iy < intervalCount; iy++)
            {
                for(int iz = 0; iz < intervalCount; iz++)
                {
                    x = XMin + static_cast<double>(ix) * dx;
                    y = YMin + static_cast<double>(iy) * dy;
                    z = ZMin + static_cast<double>(iz) * dz;

                    fVal = f(x, y, z);

                    if(localMax < fVal)
                    {
                        localMax = fVal;
                    }
                    else if(localMin > fVal)
                    {
                        localMin = fVal;
                    }
                }
            }
        }

#pragma omp critical
        {
            if(FMax < localMax)
            {
                FMax = localMax;
            }

            if(FMin > localMin)
            {
                FMin = localMin;
            }
        }
    }

    if(FMin >= FMax)
    {
        cout << "F axis margins are invalid!";
        return false;
    }

    // increase F interval by 10 percent
    double dF = (FMax - FMin) / 20.0;
    FMax = FMax + dF;
    FMin = FMin - dF;

    cout << "F Min: " << FMin << " F Max: " << FMax << endl;

    return true;
}
//=====================================================
bool inputInitData()
{
    cout << "Input X interval of integration" << endl;
    cout << "Min X: ";
    cin >> XMin;
    cout <<  "Max X: ";
    cin >> XMax;

    cout << "Input Y interval of integration" << endl;
    cout << "Min Y: ";
    cin >> YMin;
    cout <<  "Max Y: ";
    cin >> YMax;

    cout << "Input Z interval of integration" << endl;
    cout << "Min Z: ";
    cin >> ZMin;
    cout <<  "Max Z: ";
    cin >> ZMax;

    cout << "Input sample number" << endl;
    cout << "N: ";
    cin >> N;

    if(XMin >= XMax)
    {
        cout << "Integral limits for axis X are invalid!" << endl;
        return false;
    }
    else if(YMin >= YMax)
    {
        cout << "Integral limits for axis Y are invalid!" << endl;
        return false;
    }
    else if(ZMin >= ZMax)
    {
        cout << "Integral limits for axis Z are invalid!" << endl;
        return false;
    }
    else if(N <= 0)
    {
        cout << "Sample number is invalid!" << endl;
        return false;
    }

    return true;
}
//=====================================================
void calculateIntegral()
{
    int threadCount = omp_get_max_threads();
    int acceptedSampleCount = 0;

#pragma omp parallel num_threads(threadCount) reduction(+: acceptedSampleCount)
    {

        RandomPoint4DGenerator generator(omp_get_thread_num() + 1,
                                         XMin, XMax, YMin, YMax,  ZMin, ZMax, FMin, FMax);
        Point4D point;
        double fVal = 0.0;

#pragma omp for
        for(long i = 0; i < N; i++)
        {
            generator.generate(point);
            fVal = f(point);

            if(fVal > 0)
            {
                if(point.f > 0 && point.f < fVal)
                {
                    // point in positive part
                    acceptedSampleCount++;
                }
            }
            else if(fVal < 0)
            {
                if(point.f < 0 && point.f > fVal)
                {
                    // point in negative part
                    acceptedSampleCount--;
                }
            }
            // if fVal == 0  continue
        }
    }

    double integralPersentage = static_cast<double>(acceptedSampleCount)
            / static_cast<double>(N);
    integral = (XMax - XMin) * (YMax -YMin)
            * (ZMax - ZMin) * (FMax - FMin) * integralPersentage;

}
//=====================================================
void outputResult()
{
    cout << endl;
    cout << "Final evaluation of the integral: " <<  integral << endl;
    cout << endl;
}
//=====================================================
