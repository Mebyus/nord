// Gizmo Testing Framework
namespace coven::gtf {

struct Test {
    enum struct Status : u8 {
        Passed = 0,
        Failed,
    };

    // filename where test function is located
    str file;

    // name of test function
    str name;

    str recorded_error;

    Status status;

    let Test(str file, str name) noexcept : file(file), name(name), status(Status::Passed) {}

    method bool is_ok() noexcept {
        return status == Status::Passed;
    }

    // Report an error in test, marking it as failed
    method void error(str s) noexcept {
        if (!is_ok()) {
            // do not overwrite already stored error
            return;
        }

        status = Status::Failed;
        recorded_error = s;
    }

    // Print out test result. That is test location and name
    // as well as status (passed or failed) and recorded error
    method void report() noexcept {
        os::stdout.print(macro_static_str("[FAIL] "));
        os::stdout.print(file);
        os::stdout.print(macro_static_str(" # "));
        os::stdout.println(name);
        os::stdout.print(macro_static_str("    error: "));
        os::stdout.println(recorded_error);
        os::stdout.lf();
    }
};

}
