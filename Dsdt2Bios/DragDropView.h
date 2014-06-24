//
//  DragDropView.h
//  AmiInfoBoard
//
//  Created by Frédéric Geoffroy on 14/04/2014.
//  Copyright (c) 2014 FredWst. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#define EXTRACT_DSDT 1
#define EXTRACT_DSDT_AND_PATCH 2

@interface DragDropView : NSView{
    bool highlight;
    
}
@property (strong) IBOutlet NSTextView *Output;


@end
