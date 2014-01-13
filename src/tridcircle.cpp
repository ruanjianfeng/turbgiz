

#include "tridcircle.h"
#include "Wm5Transform.h"

TridCircle::TridCircle(const Circle3f& circle1, const Circle3f& circle2, const Circle3f& circle3)
{
    m_Circles[0] = circle1;
    m_Circles[1] = circle2;
    m_Circles[2] = circle3;
}

TridCircle::~TridCircle()
{
}

void TridCircle::SetCircle1(const Circle3f& circle)
{
    m_Circles[0] = circle;
}

void TridCircle::SetCircle2(const Circle3f& circle)
{
    m_Circles[1] = circle;
}

void TridCircle::SetCircle3(const Circle3f& circle)
{
    m_Circles[2] = circle;
}

Circle3f TridCircle::GetCircle(int index)
{
    return m_Circles[index];
}

bool TridCircle::GetTangentPosition(const Circle2f& circle,
                                    const Line2f& line2,
                                    Vector3f& point)
{
    // Have no DistLine2Circle2...
    Circle3f cir = ToCircle3f(circle);

    Vector3f lOri(line2.Origin.X(), line2.Origin.Y(), 0);
    Vector3f lDir(line2.Direction.X(), line2.Direction.Y(), 0);
    Line3f line3(lOri, lDir);

    DistLine3Circle3f cl(line3, cir);
    cl.GetSquared();
    Vector3f p = cl.GetClosestPoint0();
    point.X() = p.X();
    point.Y() = p.Y();
    point.Z() = p.Z();
    return true;
}

Circle2f TridCircle::ToCircle2f(const Circle3f& circle3)
{
    Vector2f center(circle3.Center.X(), circle3.Center.Y());
    return Circle2f(center, circle3.Radius);
}

Circle3f TridCircle::ToCircle3f(const Circle2f& circle2)
{
    Vector3f center(circle2.Center.X(), circle2.Center.Y(), 0);
    return Circle3f(center, Vector3f(1, 0, 0), Vector3f(0, 1, 0), Vector3f(0, 0, 1), circle2.Radius);
}

void TridCircle::GetTangentPointsToCircles(const Circle2f& cir0,
                                           const Circle2f& cir1,
                                           std::vector<Vector3f>& results)
{
    Line2f lines[4];
    if (GetTangentsToCircles(cir0, cir1, lines))
    {
        Vector3f point0, point1;
        for (int i=0; i<4; i++) {
            GetTangentPosition(cir0, lines[i], point0);
            results.push_back(point0);
            GetTangentPosition(cir1, lines[i], point1);
            results.push_back(point1);
        }
    }
}

BSplineCurve3f *TridCircle::CreateCircle()
{
    Circle2f cir0 = ToCircle2f(m_Circles[0]);
    Circle2f cir1 = ToCircle2f(m_Circles[1]);
    Circle2f cir2 = ToCircle2f(m_Circles[2]);

    // Adding samples.
    std::vector<Vector3f> allSamples;
    GetTangentPointsToCircles(cir0, cir1, allSamples);
    GetTangentPointsToCircles(cir0, cir2, allSamples);
    GetTangentPointsToCircles(cir1, cir2, allSamples);

    // Adding samples.
    int sampleNum = 20;
    TessellateCircle(cir0, sampleNum, allSamples);
    TessellateCircle(cir1, sampleNum, allSamples);
    TessellateCircle(cir2, sampleNum, allSamples);

    // Vector to array.
    int len = allSamples.size();
    Vector3f* samplePoints = new1<Vector3f>(len);
    int i=0;
    for (std::vector<Vector3f>::iterator it = allSamples.begin();
        it != allSamples.end(); it++)
    {
        samplePoints[i] = *it;
        i++;
    }

    // Compute 2D Convex hull.
    ConvexHull3f *pHull = new0 ConvexHull3f(len, samplePoints, 0.001f, false, Query::QT_REAL);
    assertion(pHull->GetDimension() == 2, "Incorrect dimension.\n");

    ConvexHull2f *pHull2 = pHull->GetConvexHull2();
    int numSimplices = pHull2->GetNumSimplices();
    const int* indices = pHull2->GetIndices();
    Vector3f* ctrlPoints = new1<Vector3f>(numSimplices);
    for (i = 0; i < numSimplices; i++)
    {
        ctrlPoints[i] = samplePoints[indices[i]];
    }

    delete0(pHull);
    delete0(pHull2);
    delete1(samplePoints);

    BSplineCurve3f *pSpline = new0 BSplineCurve3f(numSimplices, ctrlPoints, 2, true, false);
    return pSpline;
}

void TridCircle::TessellateCircle(const Circle2f& cir, int sampleNum, std::vector<Vector3f> &tess)
{
    // Start tessellate arc.
    Transform xform;
    xform.SetTranslate(Vector3f(cir.Center.X(), cir.Center.Y(), 0));

    float angle = Mathf::TWO_PI / sampleNum;
    for (int i = 0; i < sampleNum; ++i)
    {
        float x = cir.Radius * Mathf::Cos(angle * i);
        float y = cir.Radius * Mathf::Sin(angle * i);

        Vector3f position = xform * APoint(x, y, 0);
        tess.push_back(position);
    }
}

