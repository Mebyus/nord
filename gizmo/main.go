package main

import (
	"bufio"
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
)

type BuildItem struct {
	Recipes []Recipe `json:"recipes"`
	Name    string   `json:"name"`
	Kind    string   `json:"kind"`
}

type Recipe struct {
	Target  Target       `json:"target"`
	Tests   []string     `json:"tests"`
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

	BinDir   string
	CacheDir string

	Config LocalBuildConfig
}

type LocalBuildConfig struct {
	// List of build kinds:
	//
	//	- debug (debug-friendly optimizations + debug information in binaries + safety checks)
	//	- test (moderate-level optimizations + debug information in binaries + safety checks)
	//	- safe (most optimizations enabled + safety checks)
	//	- fast (all optimizations enabled + safety checks disabled)
	Kind string
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

	cmdName := args[0]
	switch cmdName {
	case "build":
	case "test":
		err := execTestCmd(args[1:])
		if err != nil {
			fatal("execute test: " + err.Error())
		}
		return
	default:
		fatal("unknown command: " + cmdName)
	}

	var buildTarget string
	if len(args) >= 2 {
		buildTarget = args[1]
	}

	var items []BuildItem
	err := readConfigFromFile(defaultBuildConfigFile, &items)
	if err != nil {
		fatal(err)
	}

	localConfig := readLocalBuildConfig()

	// verify local config
	if localConfig.Kind == "" {
		fatal("build kind cannot be empty")
	}

	switch localConfig.Kind {
	case "debug", "test", "safe", "fast":
	default:
		fatal("unknown build kind: " + localConfig.Kind)
	}

	fmt.Println("build kind:", localConfig.Kind)

	if buildTarget == "" {
		err = BuildAll(localConfig, items)
		if err != nil {
			fatal(err)
		}
		return
	}

	item, ok := findBuildItem(items, buildTarget)
	if !ok {
		fatal("build item not found")
	}
	err = Build(localConfig, item)
	if err != nil {
		fatal(err)
	}
}

func readConfigFromFile(filename string, v any) error {
	file, err := os.Open(filename)
	if err != nil {
		return err
	}
	defer file.Close()

	decoder := json.NewDecoder(file)
	return decoder.Decode(v)
}

const defaultBuildKind = "debug"

func readLocalBuildConfig() LocalBuildConfig {
	cfg := LocalBuildConfig{
		Kind: defaultBuildKind,
	}

	b, err := os.ReadFile(".build.env")
	if err != nil {
		// TODO: maybe spit some debug info about this error
		return cfg
	}

	s := bufio.NewScanner(bytes.NewReader(b))
	for s.Scan() {
		line := s.Text()
		line = strings.TrimSpace(line)
		if line == "" {
			// skip empty lines
			continue
		}
		if strings.HasPrefix(line, "#") {
			// skip line comments
			continue
		}

		split := strings.Split(line, "=")
		if len(split) != 2 {
			// TODO: report suspicious lines in config
			continue
		}

		name := strings.TrimSpace(split[0])
		value := strings.TrimSpace(split[1])

		switch name {
		case "BUILD_KIND":
			cfg.Kind = value
		default:
			// skip unknown config variables
		}
	}
	err = s.Err()
	if err != nil {
		// TODO: maybe spit some debug info about this error
		return cfg
	}

	return cfg
}

func findBuildItem(items []BuildItem, name string) (BuildItem, bool) {
	for _, item := range items {
		if item.Name == name {
			return item, true
		}
	}
	return BuildItem{}, false
}

func BuildAll(cfg LocalBuildConfig, items []BuildItem) error {
	for _, item := range items {
		err := Build(cfg, item)
		if err != nil {
			return fmt.Errorf("build %s: %w", item.Name, err)
		}
	}
	return nil
}

func Build(cfg LocalBuildConfig, item BuildItem) error {
	if len(item.Recipes) == 0 {
		return fmt.Errorf("no recipe")
	}

	objectItems := item.Recipes[0].Objects
	if len(objectItems) == 0 {
		return fmt.Errorf("no objects in recipe")
	}

	ctx := BuildContext{
		BinDir:   filepath.Join("build", item.Name, cfg.Kind, "bin"),
		CacheDir: filepath.Join("build", item.Name, cfg.Kind, "cache"),
		Config:   cfg,
	}
	err := os.MkdirAll(ctx.CacheDir, 0o775)
	if err != nil {
		return err
	}

	objectsList := make([]string, 0, len(objectItems))
	for _, objectItem := range objectItems {
		objCtx := ctx
		objCtx.FilePath = filepath.Join(ctx.CacheDir, objectItem.Name+".o")
		err := buildObject(objCtx, objectItem)
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
		fullPath := filepath.Join("src", file)
		buf.WriteString("// gizmo.file = ")
		buf.WriteString(fullPath)
		buf.WriteRune('\n')

		buf.WriteString("#line 1 \"")
		buf.WriteString(fullPath)
		buf.WriteString("\"\n")

		err := appendFileContents(&buf, fullPath)
		if err != nil {
			return err
		}
		buf.WriteRune('\n')
	}

	return os.WriteFile(outFilePath, buf.Bytes(), 0o664)
}

func linkObjects(ctx BuildContext, list []string) error {
	args := make([]string, 0, 10+len(list))

	switch ctx.Config.Kind {
	case "debug":
		args = append(args, optzFlag(debugCompilerOptimizations))
		args = append(args, debugInfoFlag)
	case "test":
		args = append(args, optzFlag(testCompilerOptimizations))
		args = append(args, debugInfoFlag)
	case "safe":
		args = append(args, "-flto")
		args = append(args, optzFlag(safeCompilerOptimizations))
	case "fast":
		args = append(args, "-flto")
		args = append(args, optzFlag(fastCompilerOptimizations))
	default:
		panic("unexpected build kind: " + ctx.Config.Kind)
	}

	args = append(args, "-o", ctx.FilePath)
	args = append(args, list...)

	// TODO: link external libraries

	cmd := exec.Command(compiler, args...)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	return cmd.Run()
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
	"-fno-rtti",
}

const compiler = "g++"
const compilerStdVersion = "20"
const debugCompilerOptimizations = "g"
const testCompilerOptimizations = "1"
const safeCompilerOptimizations = "2"
const fastCompilerOptimizations = "fast"
const debugInfoFlag = "-ggdb"
const maxCompilerErrorsFlag = "-fmax-errors=1"

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
	genFilePath := filepath.Join(ctx.CacheDir, item.Name+".gen.cc")
	err := mergeSourceFiles(genFilePath, item.Parts, item.ExtHeaders)
	if err != nil {
		return err
	}

	args := make([]string, 0, 10+len(genFlags)+len(warningFlags)+len(otherFlags)+len(item.Parts))
	args = append(args, genFlags...)
	args = append(args, maxCompilerErrorsFlag)
	args = append(args, warningFlags...)
	args = append(args, otherFlags...)
	args = append(args, stdFlag(compilerStdVersion))

	switch ctx.Config.Kind {
	case "debug":
		args = append(args, optzFlag(debugCompilerOptimizations))
		args = append(args, debugInfoFlag)
	case "test":
		args = append(args, optzFlag(testCompilerOptimizations))
		args = append(args, debugInfoFlag)
	case "safe":
		args = append(args, optzFlag(safeCompilerOptimizations))
	case "fast":
		args = append(args, optzFlag(fastCompilerOptimizations))
	default:
		panic("unexpected build kind: " + ctx.Config.Kind)
	}

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
