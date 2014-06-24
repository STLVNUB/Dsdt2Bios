
#import "DragDropView.h"
#include "c/Dsdt2Bios.h"

@implementation DragDropView


- (id)initWithFrame:(NSRect)frame{
    self = [super initWithFrame:frame];
    if (self) {
        [self registerForDraggedTypes:[NSArray arrayWithObject:NSFilenamesPboardType]];
    }
    return self;
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender{
    highlight=YES;
    [self setNeedsDisplay: YES];
    return NSDragOperationGeneric;
}

- (void)draggingExited:(id <NSDraggingInfo>)sender{
    highlight=NO;
    [self setNeedsDisplay: YES];
}

- (BOOL)prepareForDragOperation:(id <NSDraggingInfo>)sender {
    highlight=NO;
    [self setNeedsDisplay: YES];
    return YES;
}

- (BOOL)performDragOperation:(id < NSDraggingInfo >)sender {
        return YES;
}

- (void)concludeDragOperation:(id <NSDraggingInfo>)sender{
    
    NSArray *draggedFilenames = [[sender draggingPasteboard] propertyListForType:NSFilenamesPboardType];
    NSString *string;
    
    int i, ret;
    unsigned long len;
    unsigned char *d =NULL;
    unsigned short Old_Dsdt_Size, Old_Dsdt_Ofs;
    const char *FileName;
    const char *fnAmiBoardInfo = "";
    const char *fnDSDT = "";
    unsigned short reloc_padding;
    

    d = malloc(0x10000);
    
    switch ( [draggedFilenames count] ) //Number of files drop
    {
        case EXTRACT_DSDT:
            FileName = [draggedFilenames[0] UTF8String];
            ret = Read_AmiBoardInfo(FileName, d, &len, &Old_Dsdt_Size, &Old_Dsdt_Ofs,2);
            if ( ret == 2 )
                printf("\n\n\n\n\n\n\n\nFile %s has bad header\n",FileName);
            break;
            
        case EXTRACT_DSDT_AND_PATCH:
            for (i=0; i<[draggedFilenames count]; i++)
            {
                FileName = [draggedFilenames[i] UTF8String];
                ret = isDSDT(FileName);
                if (ret < 0)
                    break; // just breaking from loop
                else if (ret == 1)
                    fnDSDT = FileName;
           
                ret = isAmiBoardInfo(FileName);
                if (ret < 0)
                    break; // just breaking from loop
                else if (ret == 1)
                    fnAmiBoardInfo = FileName;
            }
            
            if(StrLength(fnAmiBoardInfo) == 0) {
                printf("\n\n\n\n\n\n\n\nAmiBoardInfo couldn't be verified\n");
                break;
            }
            
            if(StrLength(fnDSDT) == 0) {
                printf("\n\n\n\n\n\n\n\nDSDT couldn't be verified\n");
                break;
            }

            ret = Read_AmiBoardInfo(fnAmiBoardInfo,  d, &len, &Old_Dsdt_Size, &Old_Dsdt_Ofs,1);
            if(!ret) {
                printf("\n\n\n\n\n\n\n\nError while reading from AmiBoardInfo\n");
                break;
            }
            reloc_padding = 0;
            Read_Dsdt(fnDSDT, d, len, Old_Dsdt_Size, Old_Dsdt_Ofs,&reloc_padding);
            // if reloc_padding is different from 0, the main area of relocation is beyond a segment address, we must
            // therefore readjust the base address -> call it a second time with the reloc_padding parameter.
            if ( reloc_padding == 0 )
                break; // We are done already ;)
            
            ret = Read_AmiBoardInfo(fnAmiBoardInfo, d, &len, &Old_Dsdt_Size, &Old_Dsdt_Ofs,1);
            if(!ret) {
                printf("\n\n\n\n\n\n\n\nError while reading from AmiBoardInfo\n");
                break;
            }
            Read_Dsdt(fnDSDT, d, len, Old_Dsdt_Size, Old_Dsdt_Ofs,&reloc_padding);
            break;
            
        default:
            printf("\n\n\n\n\n\nYOU MUST DRAG AND DROP SIMULTANEOUSLY\n\nYOUR ORIGINAL AMIBOARDINFO AND YOUR NEW DSDT \n\nOR\n\nJUST DRAG AND DROP AMIBOARDINFO TO GET ORIGINAL DSDT");
            break;
    }
    
    string = [[NSString alloc] initWithUTF8String:cr];
    self.Output.string=string;
    free(d);
}

- (void)drawRect:(NSRect)rect{
    [super drawRect:rect];
    if ( highlight ) {
        [[NSColor greenColor] set];
        [NSBezierPath setDefaultLineWidth: 5];
        [NSBezierPath strokeRect: [self bounds]];
    }
}

@end
