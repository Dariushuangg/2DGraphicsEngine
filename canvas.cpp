#include "./include/GCanvas.h"
#include "./include/GBitmap.h"
#include"./include/GPaint.h"
#include "./include/GBlendMode.h"
#include "./include/GRect.h"
#include <vector>
#include "GEdge.h"
#include "GBlenders.h"

class Canvas : public GCanvas {
public:
    Canvas(const GBitmap& device) : fDevice(device), blenders(Blenders()) { }

    /**
     *  Fill the entire canvas with the specified color, using the specified blendmode.
     */
    void drawPaint(const GPaint& paint) override {
        blender b = blenders.getBlender(paint.getBlendMode());
        GPixel srcPixel = prepSrcPixel(paint);
        GPixel emptyPixel = GPixel_PackARGB(0, 0, 0, 0);
        GPixel result = b(srcPixel, emptyPixel);
        visit_pixels(fDevice, [&](int x, int y, GPixel* p) {
            *p = result;
        });
    }

    void drawRect(const GRect& rect, const GPaint& paint) {
        GPoint pts[4];
        pts[0] = { rect.fLeft,  rect.fTop };
        pts[1] = { rect.fRight, rect.fTop };
        pts[2] = { rect.fRight, rect.fBottom };
        pts[3] = { rect.fLeft,  rect.fBottom };
        // printf("Drawing rect: (%f, %f), (%f, %f), (%f, %f), (%f, %f) \n", rect.fLeft,  rect.fTop, rect.fRight, rect.fTop, rect.fRight, rect.fBottom, rect.fLeft,  rect.fBottom);
        drawConvexPolygon(pts, 4, paint);
    };

    void drawConvexPolygon(const GPoint points[], int count, const GPaint& paint) {
        // testClipper();
        // edgeTest();
        // Prepare edges
        vector<GEdge> edges = assembleEdges(points, count);
        std::sort(edges.begin(), edges.end(), GEdge());
        printEdges(edges);
        for (int y = 0; y < fDevice.height(); y++) {
            if (edges.empty() || edges.size() == 1) break;
            // Pick edges with the smallest y value (closer to the top of screen)
            GEdge e1 = edges[0]; //!! Opt: don't need to create instance
            GEdge e2 = edges[1];
            // printf("Picked edge 1, top %d, bot %d, m %f, b %f \n", e1.top, e1.bot, e1.m, e1.b);
            // printf("Picked edge 2, top %d, bot %d, m %f, b %f \n", e2.top, e2.bot, e2.m, e2.b);
            // if (edgeHasExpired(e1, y) || edgeHasExpired(e2, y)) assert( 1 == 1 );
            if (e2.top < e1.top) {
                GEdge temp = e2;
                e2 = e1;
                e1 = temp;
            }
            if (y < e1.top || y > e2.bot) continue;
            // Calculate left and right-most pixel covered by the shape
            int idx1 = GRoundToInt(e1.m * ((float)y + 0.5) + e1.b);
            int idx2 = GRoundToInt(e2.m * ((float)y + 0.5) + e2.b);

            // Fill the entire row of pixel between left and right index
            if (idx1 == idx2) { /* don't draw */ }
            else if (idx1 < idx2) fillRow(idx1, idx2 - 1, y, paint);
            else fillRow(idx2, idx1 - 1, y, paint);

            // Retire expired edges by removing them; We maintain the invariance
            // that edges with smallest y always has the smallest index in the array
            if (edgeHasExpired(e2, y)) edges.erase(edges.begin() + 1);
            if (edgeHasExpired(e1, y)) edges.erase(edges.begin());
        }
    };

private:
    GBitmap fDevice;
    Blenders blenders;

    void printEdges(vector<GEdge> edges) {
        // printf("size of edges is: %d \n", edges.size());
        for (int i = 0; i < edges.size(); i++) {
            // printf("Top : %d, Bot: %d, m = %f, b = %f \n", edges[i].top, edges[i].bot, edges[i].m, edges[i].b); 
        }
    }

    void edgeTest() {
        vector<GEdge> edges;

        // 1. (-1,1) (2,-1), (1,2) should be clipped to:
        // (0.000000, 0.333333) -> (0.000000, 1.000000)
        // (0.500000, 0.000000) -> (0.000000, 0.333333)
        // (1.666667, 0.000000) -> (1.000000, 2.000000)
        // (0.000000, 1.000000) -> (0.000000, 1.500000)
        // (0.000000, 1.500000) -> (1.000000, 2.000000)
        // clip(GPoint({-1, 1}), GPoint({2, -1}), edges);  
        // clip(GPoint({2, -1}), GPoint({1, 2}), edges);  
        // clip(GPoint({1, 2}), GPoint({-1, 1}), edges);  
        // printEdges(edges);

        // 2. { 3, {{0,0}, {3,3}, {1,1}} } (see pdf page 8)
        // clip(GPoint({0,0}), GPoint({3,3}), edges);  
        // clip(GPoint({3,3}), GPoint({1,1}), edges);  
        // clip(GPoint({1,1}), GPoint({0,0}), edges);  
        // printEdges(edges);

        // 3. Rect {-5, 0, 0, 3}
        // GRect rect = {-5, 0, 0, 3};
        // GPoint pts[4];
        // pts[0] = { rect.fLeft,  rect.fTop };
        // pts[1] = { rect.fRight, rect.fTop };
        // pts[2] = { rect.fRight, rect.fBottom };
        // pts[3] = { rect.fLeft,  rect.fBottom };
        // clip(pts[0], pts[1], edges);
        // clip(pts[1], pts[2], edges);
        // clip(pts[2], pts[3], edges);
        // clip(pts[3], pts[0], edges);
        // printEdges(edges);
        printf("in edge test");
        GPoint pts[12] = {
            {446.000000, 256.000000},
            {374.463074, 404.547974},
            {213.721024, 441.236298},
            {84.815903, 338.437897},
            {84.815918, 173.562088},
            {213.721008, 70.763702},
            {374.463043, 107.451996},
            {397.421326, 114.578629},
            {416.869263, 121.014496},
            {433.983795, 126.687317},
            {449.488220, 131.652496},
            {463.846008, 135.999847}
        };
        for (int i = 0; i < 11; i++) {
            clip(pts[i], pts[i + 1], edges);
        }
        clip(pts[11], pts[0], edges);
        exit(0);
    }

    /**
     * @brief For debugging clipper. Add 
     * printf("(%f, %f) -> (%f, %f) \n", p1.fX, p1.fY, p2.fX, p2.fY); return;
     * to prepGEdge.
     */
    void testClipper(){
        vector<GEdge> a;

        // error test
        clip(GPoint({-25, 45}), GPoint({-25, -25}), a);  
        exit(0);
        // 0. No clip
        // (1.000000, 3.000000) -> (2.000000, 4.000000)
        printf("0. No clip \n");
        clip(GPoint({1, 3}), GPoint({2, 4}), a);  

        // 1. Left clipper
        // (0.000000, 3.000000) -> (0.000000, 3.333333)
        // (0.000000, 3.333333) -> (2.000000, 4.000000)
        printf("1. Left clipper \n");
        clip(GPoint({-1, 3}), GPoint({2, 4}), a); 

        // 1.1 Right clipper
        printf("1.1 Right clipper \n");
        clip(GPoint({510, 4}), GPoint({513, 3}), a); 

        // 2. Left and Right clip
        // (0.000000, 3.000000) -> (0.000000, 3.001945)
        // (512.000000, 4.000000) -> (512.000000, 3.998051)
        // (0.000000, 3.001945) -> (512.000000, 3.998051)
        printf("Left and Right clip, fDevice width + 1 = %d \n", fDevice.width() + 1);
        clip(GPoint({-1, 3}), GPoint({fDevice.width() + 1, 4}), a);  

        // 3. Top clip
        // (1.428571, 0.000000) -> (2.000000, 4.000000)
        printf("3. Top clip \n");
        clip(GPoint({1, -3}), GPoint({2, 4}), a);  
        printf("3.1 Top clip \n");
        clip(GPoint({2, -1}), GPoint({1, 4}), a);  

        // 4. Bot clip
        // (1.000000, 3.000000) -> (1.998039, 512.000000)
        printf("4. Bot clip, fDevice height + 1 = %d \n", fDevice.height() + 1);
        clip(GPoint({1, 3}), GPoint({2, fDevice.height() + 1}), a);  

        // 5. Top and Bot clip
        // (1.005814, 0.000000) -> (1.998062, 512.000000) 
        printf("5. Top and Bot clip \n");
        clip(GPoint({1, -3}), GPoint({2, fDevice.height() + 1}), a);  

        exit(0);
    }

    GPixel prepSrcPixel(const GPaint& srcPaint) {
        GColor srcColor = srcPaint.getColor();
        // convert GColor floats into (un-premultiplied) integers
        int sRed = Blenders::GIntChannel(srcColor.r);
        int sGreen = Blenders::GIntChannel(srcColor.g);
        int sBlue = Blenders::GIntChannel(srcColor.b);
        int sAlpha = Blenders::GIntChannel(srcColor.a);

        int spRed = Blenders::GPreMultChannel(sRed, sAlpha);
        int spGreen = Blenders::GPreMultChannel(sGreen, sAlpha);
        int spBlue = Blenders::GPreMultChannel(sBlue, sAlpha);

        // pack the premultiplied result into GPixel
        return GPixel_PackARGB(sAlpha, spRed, spGreen, spBlue);
    }

    /**
     * @brief Determine if the scan line is the last scan line that will 
     * touch the given edge.
     */
    bool edgeHasExpired(GEdge edge, int scan) {
        if (!(scan <= edge.bot && scan >= edge.top)){
            // printf("ops!");
        }
        // assert(scan <= edge.bot && scan >= edge.top);
        return scan == edge.bot - 1;
    }

    /**
     * @brief Blend pixels between left and right index on row y with the paint.
     * 
     * @param left [Inclusive] Left-most index of the pixel included by the shape.
     * @param right [Inclusive] Right-most index of the pixel included by the shape.
     * @param row y value of the row.
     * @param paint Source paint.
     */
    void fillRow(int left, int right, int row, const GPaint& paint) {
        if (left < 0) left = 0;
        if (right < 0) right = 0;
        if (left > fDevice.width()) left = fDevice.width() - 1;
        if (right > fDevice.width()) right = fDevice.width() - 1;
        blender b = blenders.getBlender(paint.getBlendMode());
        GPixel srcPixel = prepSrcPixel(paint);
        for (int x = left; x <= right; x ++) {
            // GPixel result = b(srcPixel, emptyPixel);
            GPixel* dstPixel = fDevice.getAddr(x, row);
            GPixel blendedPixel = b(srcPixel, *dstPixel);
            *dstPixel = blendedPixel;
        }
    }

    /**
     * @brief Prepare the GEdge data structure from two GPoint points.
     * Ensure p1.Y < p2.Y.
     */
    void prepGEdge(GPoint p1, GPoint p2, vector<GEdge>& edges) {
        // printf("(%f, %f) -> (%f, %f) \n", p1.fX, p1.fY, p2.fX, p2.fY); 
        assert(p1.fY <= p2.fY);
        int top = GRoundToInt(p1.fY);
        int bot = GRoundToInt(p2.fY);
        if (top == bot) {
            // printf("top = %d, bot = %d, eliminating edge (%f, %f) -> (%f, %f) \n", top, bot, p1.fX, p1.fY, p2.fX, p2.fY); 
            return;
        } else {
            // printf("top = %d, bot = %d \n", top, bot);
        }
        float m = (p1.fX - p2.fX) / (p1.fY - p2.fY);
        float b = p1.fX - m * p1.fY;
        // printf("m = %f, b = %f \n", m, b);
        edges.push_back(GEdge({top, bot, m, b}));        
    }

    /**
     * @brief Clip edge and add the clipped edge into the list of all edges.
     */
    void clip(GPoint p1, GPoint p2, vector<GEdge>& edges) {
        // printf("clipping (%f, %f) -> (%f, %f) \n", p1.fX, p1.fY, p2.fX, p2.fY); 
        // Ensure p1 has smaller y value
        if (p2.fY < p1.fY) {
            GPoint temp = p1;
            p1 = p2;
            p2 = temp;
        }

        /* Vertical Clipping */
        // for p1
        if (p1.fY < 0) {
            if (p2.fY < 0) return; // reject top
            float ratio = (- p1.fY) / (p2.fY - p1.fY);
            float base = p2.fX - p1.fX;
            assert(ratio > 0 && ratio <= 1);
            p1.fY = 0;
            p1.fX += base * ratio;
        } 

        // for p2
        int maxHeight = fDevice.height();
        if (p2.fY > maxHeight) {
            if (p1.fY > maxHeight) return; // reject bot
            float ratio = (p2.fY - maxHeight) / (p2.fY - p1.fY);
            float base = p1.fX - p2.fX;
            assert(ratio > 0 && ratio <= 1);
            p2.fY = maxHeight;
            p2.fX += base * ratio;
        }

        /* Horizontal Clipping */
        // when prepGEdge(p3, p4, edges) is called, p3 and p4 should represent the
        // the proper newly-created vertex if clipping occurs.
        GPoint p3, p4; 
        if (p1.fX < p2.fX) { p3 = p1; p4 = p2; }
        else { p3 = p2; p4 = p1; }
        // left clipping
        if (p1.fX < 0) {
            if (p2.fX < 0) {
                GPoint proj_p1 = GPoint({0, p1.fY});
                GPoint proj_p2 = GPoint({0, p2.fY});
                prepGEdge(proj_p1, proj_p2, edges);
                return;
            }
            float ratio = (- p1.fX) / (p2.fX - p1.fX);
            float base = p2.fY - p1.fY; //!! extractable
            assert(ratio > 0 && ratio <= 1);
            p1.fX = 0;
            float p3Y = p1.fY + ratio * base;
            p3 = GPoint({0, p3Y});
            prepGEdge(p1, p3, edges);
        } 
        if (p2.fX < 0) {
            float ratio = (- p2.fX) / (p1.fX - p2.fX);
            float base = p2.fY - p1.fY;
            assert(ratio > 0 && ratio <= 1);
            p2.fX = 0;
            float p3Y = p2.fY - ratio * base;
            p3 = GPoint({0, p3Y});
            prepGEdge(p3, p2, edges);
        }

        int maxWidth = fDevice.width();
        // right clipping
        if (p1.fX > maxWidth) {
            if (p2.fX > maxWidth) {
                GPoint proj_p1 = GPoint({maxWidth, p1.fY});
                GPoint proj_p2 = GPoint({maxWidth, p2.fY});
                prepGEdge(proj_p1, proj_p2, edges);
                return;
            }
            float ratio = (p1.fX - maxWidth) / (p1.fX - p2.fX);
            float base = p2.fY - p1.fY;
            assert(ratio > 0 && ratio <= 1);
            p1.fX = maxWidth;
            float p4Y = p1.fY + ratio * base;
            p4 = GPoint({maxWidth, p4Y});
            prepGEdge(p1, p4, edges);
        }
        if (p2.fX > maxWidth) {
            float ratio = (p2.fX - maxWidth) / (p2.fX - p1.fX);
            float base = p2.fY - p1.fY;
            assert(ratio > 0 && ratio <= 1);
            p2.fX = maxWidth;
            float p4Y = p2.fY - ratio * base;
            p4 = GPoint({maxWidth, p4Y});
            prepGEdge(p4, p2, edges);
        }
        p3.fY < p4.fY ? prepGEdge(p3, p4, edges) : prepGEdge(p4, p3, edges);
    }

    /**
     * @brief Assemble points into edges by clipping all edges.
     */
    vector<GEdge> assembleEdges(const GPoint points[], int count) {
        vector<GEdge> edges;
        for (int i = 0; i < count - 1; i ++) {
            clip(points[i], points[i + 1], edges);
        }
        clip(points[count - 1], points[0], edges);
        return edges;
    }

};

std::unique_ptr<GCanvas> GCreateCanvas(const GBitmap& device) {
    return std::unique_ptr<GCanvas>(new Canvas(device));
}

std::string GDrawSomething(GCanvas* canvas, GISize dim) {
    GColor color1 = {0.5, 0.2, 0.4, 1};    
    GPaint paint1 = GPaint(color1);
    paint1.setBlendMode(GBlendMode::kDstATop);

    canvas->drawPaint(paint1);

    return "tears in rain";
}