// Smoke test: verify the editor came up and a UI tree exists.
Test.register("Smoke", [
    function check_root_exists() {
        Test.log("Smoke: starting");
        var anyEditorWidget = EditorUI.findByType("o2::Widget");
        Test.assertNotNull(anyEditorWidget, "expected at least one o2::Widget under EditorUIRoot");
        Test.log("Smoke: editor UI tree is not empty");
    },
    function dump_tree_to_log() {
        // dump goes through Test.log - only visible in verbose mode
        EditorUI.dump();
    },
    function take_screenshot() {
        Test.screenshot("smoke_initial");
    }
]);
