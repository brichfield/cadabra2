//
//  NotebookController.h
//  Cadabra
//
//  Created by Kasper Peeters on 31/01/2015.
//  Copyright (c) 2015 phi-sci. All rights reserved.
//

/// \defgroup osx OS-X 
/// \ingroup frontend
/// Notebook interface implemented using the native OS-X Cocoa toolkit.

#import <Cocoa/Cocoa.h>

@interface NotebookController : NSViewController {
}

@property (weak) IBOutlet NSTextField  *status_label;
@property (weak) IBOutlet NSScrollView *scrollbox;

@end
