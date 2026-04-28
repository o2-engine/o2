// Negative test: intentionally fails to verify the runner reports failures correctly.
// Run only when explicitly selected, e.g. --test IntentionalFail.
Test.register("IntentionalFail", [
    function step1() {
        Test.log("IntentionalFail: about to fail");
    },
    function step2() {
        Test.fail("expected failure to validate the runner");
    }
]);
