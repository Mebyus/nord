package main

import (
	"bufio"
	"bytes"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
)

// Code for managment and execution of tests written according to
// Gizmo Testing Framework

const defaultTestConfigFile = "test.json"

func execTestCmd(args []string) error {
	var items []BuildItem
	err := readConfigFromFile(defaultTestConfigFile, &items)
	if err != nil {
		fatal(err)
	}

	if len(args) == 0 {
		return execTestAll(items)
	}
	name := args[0]
	item, ok := findBuildItem(items, name)
	if !ok {
		return fmt.Errorf("test item %s not found", name)
	}
	return execTest(item)
}

func execTestAll(items []BuildItem) error {
	for _, item := range items {
		err := execTest(item)
		if err != nil {
			return fmt.Errorf("test %s: %w", item.Name, err)
		}
	}
	return nil
}

func execTest(item BuildItem) error {
	if item.Kind != "test" {
		return fmt.Errorf("not a test item")
	}

	if len(item.Recipes) == 0 {
		return fmt.Errorf("no recipe")
	}

	recipe := item.Recipes[0]
	objectItems := recipe.Objects
	if len(objectItems) == 0 {
		return fmt.Errorf("no objects in recipe")
	}

	if len(recipe.Tests) == 0 {
		return fmt.Errorf("no test files in recipe")
	}

	var ok bool
	for _, objectItem := range objectItems {
		if objectItem.Name == "main" && objectItem.Kind == "c++" {
			ok = true
			break
		}
	}
	if !ok {
		return fmt.Errorf("no main object in recipe")
	}

	ctx := BuildContext{
		BinDir:   filepath.Join("build", item.Name+".test", "bin"),
		CacheDir: filepath.Join("build", item.Name+".test", "cache"),
	}
	err := os.MkdirAll(ctx.CacheDir, 0o775)
	if err != nil {
		return err
	}

	objectsList := make([]string, 0, len(objectItems))
	for _, objectItem := range objectItems {
		objCtx := ctx
		objCtx.FilePath = filepath.Join(ctx.CacheDir, objectItem.Name+".o")

		if objectItem.Name == "main" && objectItem.Kind == "c++" {
			err = buildTestMainObject(objCtx, objectItem, recipe.Tests)
		} else {
			err = buildObject(objCtx, objectItem)
		}
		if err != nil {
			return fmt.Errorf("build %s object %s: %w", objectItem.Kind, objectItem.Name, err)
		}

		objectsList = append(objectsList, objCtx.FilePath)
	}

	err = os.MkdirAll(ctx.BinDir, 0o775)
	if err != nil {
		return err
	}

	ctx.FilePath = filepath.Join(ctx.BinDir, item.Name)
	return linkObjects(ctx, objectsList)
}

func buildTestMainObject(ctx BuildContext, item ObjectItem, testFilenames []string) error {
	genFilePath := filepath.Join(ctx.CacheDir, item.Name+".gen.cc")
	err := mergeTestFiles(genFilePath, item.Parts, testFilenames, item.ExtHeaders)
	if err != nil {
		return err
	}

	args := make([]string, 0, 10+len(genFlags)+len(warningFlags)+len(otherFlags)+len(item.Parts))
	args = append(args, genFlags...)
	args = append(args, warningFlags...)
	args = append(args, otherFlags...)
	args = append(args, stdFlag(compilerStdVersion))

	args = append(args, optzFlag(debugCompilerOptimizations))
	args = append(args, debugInfoFlag)

	args = append(args, "-o", ctx.FilePath)
	args = append(args, "-c", genFilePath)

	cmd := exec.Command(compiler, args...)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	return cmd.Run()
}

func mergeTestFiles(outFilePath string, files []string, testFiles []string, includeHeaders []string) error {
	buf := bytes.Buffer{}

	for _, header := range includeHeaders {
		buf.WriteString("#include <")
		buf.WriteString(header)
		buf.WriteString(">\n")
	}

	buf.WriteRune('\n')

	for _, file := range files {
		fullPath := filepath.Join("src", file)
		buf.WriteString("// gizmo.file = ")
		buf.WriteString(fullPath)
		buf.WriteRune('\n')

		err := appendFileContents(&buf, fullPath)
		if err != nil {
			return err
		}
		buf.WriteRune('\n')
	}

	var funcList []string
	for _, file := range testFiles {
		fullPath := filepath.Join("src", file)
		buf.WriteString("// gizmo.file = ")
		buf.WriteString(fullPath)
		buf.WriteRune('\n')

		list, err := appendTestFileContents(&buf, fullPath)
		if err != nil {
			return err
		}
		buf.WriteRune('\n')

		funcList = append(funcList, list...)
	}

	appendTestMainFunc(&buf, funcList)

	return os.WriteFile(outFilePath, buf.Bytes(), 0o664)
}

func appendTestMainFunc(buf *bytes.Buffer, funcList []string) {
	buf.WriteString("fn i32 main() noexcept {\n")
	buf.WriteString("    var usz failed_tests_counter = 0;\n\n")

	for _, funcFullName := range funcList {
		buf.WriteString("    var coven::gtf::Test t = coven::gtf::Test(str(), macro_static_str(\"" + funcFullName + "\"));\n")
		buf.WriteString("    " + funcFullName + "(t);\n")
		buf.WriteString("    if (!t.is_ok()) {\n")
		buf.WriteString("        failed_tests_counter += 1;\n")
		buf.WriteString("        t.report();\n")
		buf.WriteString("    }\n")
		buf.WriteRune('\n')
	}

	buf.WriteString("    os::stdout.flush();\n")
	buf.WriteRune('\n')
	buf.WriteString("    if (failed_tests_counter != 0) {\n")
	buf.WriteString("        return 1;\n")
	buf.WriteString("    }\n")
	buf.WriteRune('\n')
	buf.WriteString("    return 0;\n")
	buf.WriteString("}\n")
}

func appendTestFileContents(buf *bytes.Buffer, path string) (funcList []string, err error) {
	file, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	var namespace string
	s := bufio.NewScanner(file)
	for s.Scan() {
		rawLineText := s.Text()
		line := strings.TrimSpace(rawLineText)

		if strings.HasPrefix(line, "#test") {
			// add test function to list, but do not
			// output this line into buffer
			split := strings.Fields(line)
			if len(split) < 2 {
				return nil, fmt.Errorf("bad #test line: %s", line)
			}
			funcName := split[1]
			funcList = append(funcList, namespace+"::"+funcName)
			continue
		}

		if strings.HasPrefix(line, "namespace") {
			split := strings.Fields(line)
			if len(split) < 2 {
				return nil, fmt.Errorf("bad namespace line: %s", line)
			}
			namespace = split[1]
		}

		buf.WriteString(rawLineText)
		buf.WriteRune('\n')
	}
	err = s.Err()
	if err != nil {
		// TODO: maybe spit some debug info about this error
		return nil, err
	}

	return funcList, nil
}
