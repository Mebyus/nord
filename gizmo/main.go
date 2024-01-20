package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"os"
	"os/exec"
	"path/filepath"
)

type BuildItem struct {
	Recipes []Recipe `json:"recipes"`
	Name    string   `json:"name"`
	Kind    string   `json:"kind"`
}

type Recipe struct {
	Target  Target       `json:"target"`
	Objects []ObjectItem `json:"objects"`
}

type Target struct {
	OS   string `json:"os"`
	Arch string `json:"arch"`
}

type ObjectItem struct {
	Name       string   `json:"name"`
	Kind       string   `json:"kind"`
	Parts      []string `json:"parts"`
	ExtHeaders []string `json:"ext_headers"`
}

type BuildContext struct {
	FilePath string
	SaveDir  string
}

const defaultBuildConfigFile = "build.json"

func fatal(v any) {
	fmt.Fprintln(os.Stderr, v)
	os.Exit(1)
}

func main() {
	args := os.Args[1:]
	if len(args) == 0 {
		fatal("no command specified")
	}

	if len(args) >= 1 && args[0] != "build" {
		fatal("unknown command")
	}

	var buildTarget string
	if len(args) == 2 {
		buildTarget = args[1]
	}

	b, err := os.ReadFile(defaultBuildConfigFile)
	if err != nil {
		fatal(err)
	}

	var items []BuildItem
	err = json.Unmarshal(b, &items)
	if err != nil {
		fatal(err)
	}

	if buildTarget == "" {
		err = BuildAll(items)
		if err != nil {
			fatal(err)
		}
		return
	}

	item, ok := findBuildItem(items, buildTarget)
	if !ok {
		fatal("build item not found")
	}
	err = Build(item)
	if err != nil {
		fatal(err)
	}
}

func findBuildItem(items []BuildItem, name string) (BuildItem, bool) {
	for _, item := range items {
		if item.Name == name {
			return item, true
		}
	}
	return BuildItem{}, false
}

func BuildAll(items []BuildItem) error {
	for _, item := range items {
		err := Build(item)
		if err != nil {
			return fmt.Errorf("build %s: %w", item.Name, err)
		}
	}
	return nil
}

func Build(item BuildItem) error {
	if len(item.Recipes) == 0 {
		return fmt.Errorf("no recipe")
	}

	objectItems := item.Recipes[0].Objects
	if len(objectItems) == 0 {
		return fmt.Errorf("no objects in recipe")
	}

	ctx := BuildContext{SaveDir: filepath.Join("build", item.Name, "release/cache")}
	err := os.MkdirAll(ctx.SaveDir, 0o775)
	if err != nil {
		return err
	}

	objectsList := make([]string, 0, len(objectItems))
	for _, objectItem := range objectItems {
		objCtx := ctx
		objCtx.FilePath = filepath.Join(ctx.SaveDir, objectItem.Name+".o")
		err := buildObject(objCtx, objectItem)
		if err != nil {
			return fmt.Errorf("build %s object %s: %w", objectItem.Kind, objectItem.Name, err)
		}

		objectsList = append(objectsList, objCtx.FilePath)
	}

	ctx = BuildContext{SaveDir: filepath.Join("build", item.Name, "release/bin")}
	ctx.FilePath = filepath.Join(ctx.SaveDir, item.Name)
	return linkObjects(ctx, objectsList)
}

func appendFileContents(buf *bytes.Buffer, path string) error {
	f, err := os.Open(path)
	if err != nil {
		return err
	}
	defer f.Close()

	_, err = io.Copy(buf, f)
	return err
}

func mergeSourceFiles(outFilePath string, files []string, includeHeaders []string) error {
	buf := bytes.Buffer{}

	for _, header := range includeHeaders {
		buf.WriteString("#include <")
		buf.WriteString(header)
		buf.WriteString(">\n")
	}

	buf.WriteRune('\n')

	for _, file := range files {
		err := appendFileContents(&buf, filepath.Join("src", file))
		if err != nil {
			return err
		}
		buf.WriteRune('\n')
	}

	return os.WriteFile(outFilePath, buf.Bytes(), 0o664)
}

func linkObjects(ctx BuildContext, list []string) error {
	return nil
}

func buildObject(ctx BuildContext, item ObjectItem) error {
	switch item.Kind {
	case "c++":
		return buildCppObject(ctx, item)
	case "asm":
		return buildAsmObject(ctx, item)
	default:
		return fmt.Errorf("unknown object kind=%s", item.Kind)
	}
}

var warningFlags = []string{
	"-Wall",
	"-Wextra",
	"-Wconversion",
	"-Wunreachable-code",
	"-Wshadow",
	"-Wundef",
	"-Wfloat-equal",
	"-Wformat=0",
	"-Wpointer-arith",
	"-Winit-self",
	"-Wduplicated-branches",
	"-Wduplicated-cond",
	"-Wnull-dereference",
	"-Wswitch-enum",
	"-Wvla",
	"-Wnoexcept",
	"-Wswitch-default",
	"-Wno-main",
	"-Wno-shadow",
	"-Wshadow=local",
}

var genFlags = []string{
	"-fwrapv",
	"-fno-exceptions",
}

const compiler = "g++"
const compilerStdVersion = "20"
const compilerOptimizations = "2"
const debugInfoFlag = "-ggdb"

var otherFlags = []string{
	"-Werror",
	"-pipe",
}

func stdFlag(v string) string {
	return "-std=c++" + v
}

func optzFlag(v string) string {
	return "-O" + v
}

func buildCppObject(ctx BuildContext, item ObjectItem) error {
	genFilePath := filepath.Join(ctx.SaveDir, item.Name+".gen.cc")
	err := mergeSourceFiles(genFilePath, item.Parts, item.ExtHeaders)
	if err != nil {
		return err
	}

	args := make([]string, 0, 2+len(genFlags)+len(warningFlags)+len(otherFlags)+len(item.Parts))
	args = append(args, genFlags...)
	args = append(args, warningFlags...)
	args = append(args, otherFlags...)
	args = append(args, stdFlag(compilerStdVersion))
	args = append(args, debugInfoFlag)
	args = append(args, "-o", ctx.FilePath)
	args = append(args, "-c", genFilePath)

	cmd := exec.Command(compiler, args...)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	return cmd.Run()
}

func buildAsmObject(ctx BuildContext, item ObjectItem) error {
	args := make([]string, 0, 2+len(item.Parts))
	args = append(args, "-o", ctx.FilePath)
	for _, part := range item.Parts {
		args = append(args, filepath.Join("src", part))
	}

	cmd := exec.Command("as", args...)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	return cmd.Run()
}
