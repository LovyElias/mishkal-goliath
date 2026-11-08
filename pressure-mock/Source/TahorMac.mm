// Mishkal Tahor V3 - full screen on the Mac: size the editor to the screen the plug-in window is on (leaving the host's
// own title / tool bar room) and pin the window's top to the top of that screen, so nothing is cut off
#import <Cocoa/Cocoa.h>

float tahorMacFullScale (void* nsView, float W, float H, float curW, float curH)
{
    NSView* v = (NSView*) nsView;
    NSWindow* w = [v window];
    if (w == nil) return 0.0f;
    const NSRect vis = [[w screen] visibleFrame];
    const NSRect fr = [w frame];
    const float chromeH = (float) fr.size.height - curH, chromeW = (float) fr.size.width - curW;   // the host's own bars
    const float s = fminf (((float) vis.size.width - fmaxf (0.0f, chromeW)) / W, ((float) vis.size.height - fmaxf (0.0f, chromeH)) / H);
    return s;
}

void tahorMacPinTop (void* nsView)
{
    NSView* v = (NSView*) nsView;
    NSWindow* w = [v window];
    if (w == nil) return;
    const NSRect vis = [[w screen] visibleFrame];
    [w setFrameTopLeftPoint: NSMakePoint (vis.origin.x, vis.origin.y + vis.size.height)];
}
