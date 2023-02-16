#include "./include/GPixel.h"
#include "./include/GBlendMode.h"
#include <unordered_map>

using namespace std;

typedef GPixel(*blender)(const GPixel&, const GPixel&); // (GPixel src, GPixel dst)

    // kClear,    //!<     0
    // kSrc,      //!<     S
    // kDst,      //!<     D
    // kSrcOver,  //!<     S + (1 - Sa)*D
    // kDstOver,  //!<     D + (1 - Da)*S
    // kSrcIn,    //!<     Da * S
    // kDstIn,    //!<     Sa * D
    // kSrcOut,   //!<     (1 - Da)*S
    // kDstOut,   //!<     (1 - Sa)*D
    // kSrcATop,  //!<     Da*S + (1 - Sa)*D
    // kDstATop,  //!<     Sa*D + (1 - Da)*S
    // kXor,      //!<     (1 - Sa)*D + (1 - Da)*S

struct Blenders {
    unordered_map<GBlendMode, blender> blenders;

    Blenders() {
        blenders[GBlendMode::kClear] = &blendClear; 
        blenders[GBlendMode::kSrc] = &blendSrc;
        blenders[GBlendMode::kDst] = &blendDst;
        blenders[GBlendMode::kSrcOver] = &blendSrcOver;
        blenders[GBlendMode::kDstOver] = &blendDstOver;
        blenders[GBlendMode::kSrcIn] = &blendSrcIn;
        blenders[GBlendMode::kDstIn] = &blendDstIn;
        blenders[GBlendMode::kSrcOut] = &blendSrcOut;
        blenders[GBlendMode::kDstOut] = &blendDstOut;
        blenders[GBlendMode::kSrcATop] = &blendSrcATop;
        blenders[GBlendMode::kDstATop] = &blendDstATop;
        blenders[GBlendMode::kXor] = &blendXor;
    }
    
    blender getBlender(GBlendMode mode) {
        return blenders[mode];
    }   

        /**
     * @brief Return the value of pixel * (numerator / 255).
     * Observe that operations on each of the color channel are identical. Thus, 
     * we use this function to compute them in parallel. The idea is to store bit
     * representations of all 4 channels into one 64-bit variable, and do the shifting
     * in that 64-bit variable.
    */
    static inline GPixel parallel_mult_diff255(uint32_t pixel, uint8_t numerator){
        uint64_t res = expand_to_64(pixel) * numerator;
        res += parallel_add(128);			
        res += (res >> 8) & parallel_add(0xFF);
        res >>= 8;
        return compress_to_32(res);
    }

    /**
     * @brief Turn 0xXX into 0x00XX00XX00XX00XX; Return a 64-bit value such that each when
     * added to, each channel get added by x.
    */
    static inline uint64_t parallel_add(uint64_t x) {
        return (x << 48) | (x << 32) | (x << 16) | x;
    }

    /**
     * Expand the 32-bit 0xAABBCCDD into 0x__AA__CC__BB__DD so that we can shift bits
     * without overflowing the variable.
    */
    static inline uint64_t expand_to_64(uint32_t x) {
        uint64_t AG = x & 0xFF00FF00;  
        uint64_t RB = x & 0x00FF00FF; 
        return (AG << 24) | RB;
    }

    static inline unsigned div255(unsigned x) {
        x += 128;
        return (x << 8) + x >> 16;
    } 

    /**
     * Compress the temporary 64-bit 0x__AA__CC__BB__DD back into 32-bit 0xAABBCCDD.
    */
    static inline uint32_t compress_to_32(uint64_t x) {
        return ((x >> 24) & 0xFF00FF00) | (x & 0x00FF00FF);
    }

    /**
     * Converts a float [0,1] representation of R/G/B/A channel to integer
     * [0,256] representation of R/G/B/A channel.
    */
    static inline int GIntChannel(float channel) {
        float unroundedFloatColor = channel * 255;
        return GRoundToInt(unroundedFloatColor);
    }

    /**
     * Premultiply the new color with the new alpha.
    */
    static inline unsigned int GPreMultChannel(int intChannel, int alpha) {
        return div255(intChannel * alpha);
    }

    static inline GPixel blendClear(const GPixel& src, const GPixel& dst) { 
        return GPixel_PackARGB(0, 0, 0, 0); 
    }

    static inline GPixel blendSrc(const GPixel& src, const GPixel& dst) { return src; }

    static inline GPixel blendDst(const GPixel& src, const GPixel& dst) { return dst; }

    static inline GPixel blendSrcOver(const GPixel& src, const GPixel& dst) {
        return src + parallel_mult_diff255(dst, 255 - GPixel_GetA(src));
    }

    static inline GPixel blendDstOver(const GPixel& src, const GPixel& dst) {
        return dst + parallel_mult_diff255(src, 255 - GPixel_GetA(dst));
    }

    static inline GPixel blendSrcIn(const GPixel& src, const GPixel& dst) {
        return parallel_mult_diff255(src, GPixel_GetA(dst));
    }

    static inline GPixel blendDstIn(const GPixel& src, const GPixel& dst) {
        return parallel_mult_diff255(dst, GPixel_GetA(src));
    }

    static inline GPixel blendSrcOut(const GPixel& src, const GPixel& dst) {
        return parallel_mult_diff255(src, 255 - GPixel_GetA(dst));
    }

    static inline GPixel blendDstOut(const GPixel& src, const GPixel& dst) {
        return parallel_mult_diff255(dst, 255 - GPixel_GetA(src));
    }

    static inline GPixel blendSrcATop(const GPixel& src, const GPixel& dst) {
        return parallel_mult_diff255(src, GPixel_GetA(dst)) 
        + parallel_mult_diff255(dst, 255 - GPixel_GetA(src));
    }

    static inline GPixel blendDstATop(const GPixel& src, const GPixel& dst) {
        return parallel_mult_diff255(dst, GPixel_GetA(src)) 
        + parallel_mult_diff255(src, 255 - GPixel_GetA(dst));
    }

    static inline GPixel blendXor(const GPixel& src, const GPixel& dst) {
        return parallel_mult_diff255(dst, 255 - GPixel_GetA(src)) 
        + parallel_mult_diff255(src, 255 - GPixel_GetA(dst));
    }
};


