#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>

#import "intro_video.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

@interface IntroDecoder : NSObject
@property (nonatomic, strong) AVAssetReader *reader;
@property (nonatomic, strong) AVAssetReaderTrackOutput *trackOutput;
@property (nonatomic, assign) float frameDt;
@property (nonatomic, assign) double durationSecs;
- (BOOL)configureWithPath:(NSString *)path outW:(int)outW outH:(int)outH;
@end

static BOOL IvSyncLoadAssetKeys(AVAsset *asset) {
    __block BOOL loaded = NO;
    dispatch_semaphore_t sem = dispatch_semaphore_create(0);
    NSArray *keys = @[ @"tracks", @"duration" ];
    [asset loadValuesAsynchronouslyForKeys:keys completionHandler:^{
        NSError *e = nil;
        AVKeyValueStatus st = [asset statusOfValueForKey:@"tracks" error:&e];
        loaded = (st == AVKeyValueStatusLoaded);
        dispatch_semaphore_signal(sem);
    }];
    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
    return loaded;
}

@implementation IntroDecoder
- (BOOL)configureWithPath:(NSString *)path outW:(int)outW outH:(int)outH {
    if (outW < 2 || outH < 2) return NO;
    if (![[NSFileManager defaultManager] fileExistsAtPath:path]) return NO;

    NSURL *url = [NSURL fileURLWithPath:path];
    AVURLAsset *asset = [AVURLAsset URLAssetWithURL:url options:nil];
    if (!IvSyncLoadAssetKeys(asset))
        return NO;

    NSArray<AVAssetTrack *> *vtracks = [asset tracksWithMediaType:AVMediaTypeVideo];
    if (vtracks.count == 0) return NO;
    AVAssetTrack *vtrack = vtracks.firstObject;

    CGSize natural = vtrack.naturalSize;
    CGAffineTransform tf = vtrack.preferredTransform;
    CGSize disp = CGSizeApplyAffineTransform(natural, tf);
    float vw = fabs((double)disp.width);
    float vh = fabs((double)disp.height);
    if (vw < 1.f || vh < 1.f) return NO;

    NSDictionary *outSettings = @{
        (id)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA),
        (id)kCVPixelBufferWidthKey : @(outW),
        (id)kCVPixelBufferHeightKey : @(outH),
    };

    NSError *err = nil;
    self.reader = [AVAssetReader assetReaderWithAsset:asset error:&err];
    if (!self.reader || err) return NO;

    self.trackOutput = [AVAssetReaderTrackOutput assetReaderTrackOutputWithTrack:vtrack outputSettings:outSettings];
    if (!self.trackOutput) return NO;
    self.trackOutput.alwaysCopiesSampleData = NO;

    [self.reader addOutput:self.trackOutput];
    if (![self.reader startReading])
        return NO;

    float fps = (float)vtrack.nominalFrameRate;
    if (fps < 1.f || fps > 120.f)
        fps = 30.f;
    self.frameDt = 1.f / fps;
    self.durationSecs = CMTimeGetSeconds(asset.duration);
    if (!isfinite(self.durationSecs) || self.durationSecs < 0.1)
        self.durationSecs = 1.0;

    return YES;
}
@end

struct IntroVideoPlayer {
    void *decoderObj;
    Texture2D tex;
    Image img;
    double mediaTime;
    double nextFrameDue;
    float tailHold;
    BOOL drained;
};

static void CopyBGRABytesToImage(const uint8_t *src, int srcStride, int w, int h, Image *img) {
    /* CVPixelBuffer BGRA rows are top-to-bottom; Raylib Image matches (no GL-style Y flip). */
    for (int y = 0; y < h; y++) {
        const uint8_t *row = src + y * srcStride;
        uint8_t *drow = (uint8_t *)img->data + (size_t)y * (size_t)w * 4;
        for (int x = 0; x < w; x++) {
            const uint8_t *p = row + x * 4;
            drow[x * 4 + 0] = p[2];
            drow[x * 4 + 1] = p[1];
            drow[x * 4 + 2] = p[0];
            drow[x * 4 + 3] = p[3];
        }
    }
}

static BOOL UploadSampleBuffer(struct IntroVideoPlayer *p, CMSampleBufferRef sb) {
    if (!p || !sb) return NO;
    CVImageBufferRef pix = CMSampleBufferGetImageBuffer(sb);
    if (!pix) return NO;

    CVPixelBufferLockBaseAddress(pix, kCVPixelBufferLock_ReadOnly);
    size_t w = CVPixelBufferGetWidth(pix);
    size_t h = CVPixelBufferGetHeight(pix);
    size_t rowBytes = CVPixelBufferGetBytesPerRow(pix);
    void *base = CVPixelBufferGetBaseAddress(pix);

    BOOL ok = NO;
    if (base && w == (size_t)p->img.width && h == (size_t)p->img.height) {
        CopyBGRABytesToImage(base, (int)rowBytes, (int)w, (int)h, &p->img);
        UpdateTexture(p->tex, p->img.data);
        ok = YES;
    }
    CVPixelBufferUnlockBaseAddress(pix, kCVPixelBufferLock_ReadOnly);
    return ok;
}

IntroVideoPlayer *IntroVideo_Open(const char *pathUtf8, int screenW, int screenH) {
    if (!pathUtf8 || screenW < 16 || screenH < 16) return NULL;

    @autoreleasepool {
        NSString *path = [NSString stringWithUTF8String:pathUtf8];
        if (!path) return NULL;

        AVURLAsset *probe = [AVURLAsset URLAssetWithURL:[NSURL fileURLWithPath:path] options:nil];
        if (!IvSyncLoadAssetKeys(probe))
            return NULL;
        NSArray *tracks = [probe tracksWithMediaType:AVMediaTypeVideo];
        if (tracks.count == 0) return NULL;
        AVAssetTrack *vtrack = tracks.firstObject;
        CGSize natural = vtrack.naturalSize;
        CGAffineTransform tf = vtrack.preferredTransform;
        CGSize disp = CGSizeApplyAffineTransform(natural, tf);
        float vw = fabs((double)disp.width);
        float vh = fabs((double)disp.height);
        if (vw < 1.f || vh < 1.f) return NULL;

        float scale = fminf((float)screenW / vw, (float)screenH / vh);
        int outW = (int)(vw * scale + 0.5f);
        int outH = (int)(vh * scale + 0.5f);
        if (outW < 2) outW = 2;
        if (outH < 2) outH = 2;

        IntroDecoder *dec = [[IntroDecoder alloc] init];
        if (![dec configureWithPath:path outW:outW outH:outH])
            return NULL;

        struct IntroVideoPlayer *p = malloc(sizeof(struct IntroVideoPlayer));
        if (!p) return NULL;
        memset(p, 0, sizeof(*p));
        p->decoderObj = (void *)CFBridgingRetain((id)dec);

        p->img = GenImageColor(outW, outH, (Color){ 0, 0, 0, 255 });
        p->tex = LoadTextureFromImage(p->img);

        /* First frame */
        CMSampleBufferRef sb = [dec.trackOutput copyNextSampleBuffer];
        if (!sb) {
            UnloadTexture(p->tex);
            UnloadImage(p->img);
            free(p);
            return NULL;
        }
        UploadSampleBuffer(p, sb);
        CFRelease(sb);

        p->nextFrameDue = dec.frameDt;
        p->mediaTime = 0.0;
        return p;
    }
}

void IntroVideo_Close(IntroVideoPlayer *p) {
    if (!p) return;
    if (p->decoderObj) {
        id held = (__bridge_transfer id)p->decoderObj;
        p->decoderObj = NULL;
        IntroDecoder *dec = (IntroDecoder *)held;
        if (dec.reader)
            [dec.reader cancelReading];
        (void)dec;
    }
    if (p->tex.id != 0)
        UnloadTexture(p->tex);
    if (p->img.data != NULL)
        UnloadImage(p->img);
    p->tex = (Texture2D){ 0 };
    p->img = (Image){ 0 };
    free(p);
}

bool IntroVideo_Update(IntroVideoPlayer *p, float dt) {
    if (!p || !p->decoderObj) return false;

    IntroDecoder *dec = (__bridge IntroDecoder *)p->decoderObj;
    p->mediaTime += (double)dt;

    while (p->mediaTime >= p->nextFrameDue && !p->drained) {
        CMSampleBufferRef sb = [dec.trackOutput copyNextSampleBuffer];
        if (!sb) {
            p->drained = YES;
            p->tailHold = 0.45f;
            break;
        }
        UploadSampleBuffer(p, sb);
        CFRelease(sb);
        p->nextFrameDue += dec.frameDt;
    }

    if (p->drained) {
        p->tailHold -= dt;
        return p->tailHold > 0.f;
    }
    return true;
}

Texture2D IntroVideo_GetTexture(const IntroVideoPlayer *p) {
    if (!p) return (Texture2D){ 0 };
    return p->tex;
}
