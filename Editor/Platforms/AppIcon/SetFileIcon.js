// Sets the Finder icon of a file: osascript -l JavaScript SetFileIcon.js <icon.icns> <file>
ObjC.import('AppKit');

function run(argv) {
    const image = $.NSImage.alloc.initWithContentsOfFile(argv[0]);
    $.NSWorkspace.sharedWorkspace.setIconForFileOptions(image, argv[1], 0);
}
