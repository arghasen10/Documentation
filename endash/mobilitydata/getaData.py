### current bpython session - make changes and save to reevaluate session.
### lines beginning with ### will be ignored.
### To return to bpython without reevaluating make no changes to this file
### or save an empty file.
import math

data = "732.95,1155.37 721.54,1153.53 710.00,1151.81 698.58,1150.09 686.80,1148.25 676.12,1146.66 664.09,1144.94 650.96,1142.97 638.07,1140.64 626.04,1138.31 612.29,1135.85 599.53,1133.40 587.13,1130.58 576.82,1127.63 564.79,1124.44 554.85,1120.76 543.18,1116.46 531.15,1112.16 520.23,1106.52 509.55,1100.26 497.77,1093.26 490.52,1085.16 490.52,1085.16 482.67,1075.46 475.92,1065.76 470.27,1056.31 464.75,1046.98 465.12,1046.74 457.63,1034.83 453.33,1016.79 450.88,1003.16 452.23,986.71 454.07,972.23 456.40,959.83 459.84,948.29 463.52,935.41 469.17,921.41 473.71,910.98 478.50,898.83 482.91,887.90 484.76,882.99 493.72,875.75 498.63,871.70 504.89,867.89 517.16,863.11 528.21,858.20 538.64,853.78 549.20,849.36 559.39,844.82 571.91,838.43 575.59,836.71 586.51,834.26 600.39,830.70 603.70,829.96 615.48,829.60 627.88,829.47 641.87,831.56 655.75,833.77 675.88,836.71 788.44,853.29 831.15,860.28 833.98,859.67 836.68,861.26 901.86,874.89 1041.79,903.98 1105.62,916.87 1115.44,920.43 1121.70,929.88 1179.39,1036.67 1242.00,1152.18 1269.25,1201.77 1268.14,1213.19 1261.39,1226.93 1257.46,1223.62 1170.31,1216.26 1120.35,1212.20 964.21,1192.69 958.32,1192.32 950.47,1190.97 786.35,1163.60 740.20,1156.48 732.95,1155.37"

points = [[float(x) for x in pt.split(',')] for pt in data.split(' ')]
points[0] == points[-1]
def getPoint(pt1, pt2, pt3, r):
    m = (pt2[1] - pt1[1])/(pt2[0] - pt1[0])
    c = pt1[1] - m * pt1[0]
    A = (1 + m)**2
    B = 2*(m*c - m*pt3[1] - pt3[0])
    C = (pt3[1] - c)**2 + pt3[1]**2 - r**2

    print(A, B, C)
    print(B**2 - 4 * A * C)
    x1 = (-B + math.sqrt(B**2 - 4 * A * C))/(2*A)
    x2 = (-B - math.sqrt(B**2 - 4 * A * C))/(2*A)
    y1 = m * x1 + c
    y2 = m * x2 + c
    return [[x1,y1], [x2,y2]]


def getNextPoint(pt, points, md=48):
    assert len(points) > 0
    pt1 = pt
    i = 0
    while True:
        if len(points) == i:
            return pt1, []
        pt2 = points[i]
        if pt2 == pt1:
            i += 1
            continue
        d = math.sqrt((pt1[0] - pt2[0])**2 + (pt1[1] - pt2[1])**2)
        d = round(d, 2)
        xm = (pt2[0] - pt1[0])/d
        ym = (pt2[1] - pt1[1])/d
        if (d - md) < -0.5:
            md -= d
            i += 1
            pt1 = pt2
            continue
        if (d - md) < 0.5:
            return pt2, points[i+1:]
        px = round(pt1[0] + xm * md, 2)
        py = round(pt1[1] + ym * md, 2)
        return [px,py], points[i:]

newPoints = []
pt = points[0]
points = points[1:]
if pt == points[-1]:
    points = points[:-1]

newPoints += [pt]
while len(points):
    pt, points = getNextPoint(pt, points)
    newPoints += [pt]

for p in newPoints:
    print(*p)
