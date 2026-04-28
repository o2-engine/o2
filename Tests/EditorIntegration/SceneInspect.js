// Verify Scene API is reachable from JS and dump tree to log.
Test.register("SceneInspect", [
    function dump_scene() {
        Scene.dump();
        Test.log("SceneInspect: scene dumped to log");
    }
]);
