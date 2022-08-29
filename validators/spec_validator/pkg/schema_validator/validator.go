package schemavalidator

import (
	"encoding/json"
	"fmt"
	"io/fs"
	"io/ioutil"
	"log"
	"os"
	"path"
	"path/filepath"

	js "github.com/santhosh-tekuri/jsonschema/v5"
)

func getSchemaURL(schemaPath string) (string, error) {
	if _, err := os.Lstat(schemaPath); err != nil {
		return "", fmt.Errorf("no such folder: %s", schemaPath)
	}

	data, err := ioutil.ReadFile(schemaPath)
	if err != nil {
		return "", fmt.Errorf("failed to read the spec file: %s, %w", schemaPath, err)
	}

	var content struct {
		Url string `json:"$id"`
	}

	if err := json.Unmarshal(data, &content); err != nil {
		return "", fmt.Errorf("input schema for <%s> was invalid json or missing $id: %w", schemaPath, err)
	}

	return content.Url, nil
}

type SchemaValidator struct {
	s *js.Schema
}

// New makes a new SchemaValidator given the path to a root schema json file
func New(schemaPath string) (*SchemaValidator, error) {
	c := js.NewCompiler()

	err := filepath.WalkDir(path.Dir(schemaPath), func(fpath string, d fs.DirEntry, err error) error {
		if !d.IsDir() && filepath.Ext(fpath) == ".json" {
			url, err := getSchemaURL(fpath)
			if err != nil {
				return fmt.Errorf("failed to get schema $id: %w", err)
			}

			f, err := os.Open(fpath)
			if err != nil {
				return fmt.Errorf("could not open extension spec: %v", err)
			}
			defer f.Close()

			if err := c.AddResource(url, f); err != nil {
				return err
			}
			log.Printf("Registered ext schema %v -> %v\n", url, fpath)
		}

		return nil
	})
	if err != nil {
		return nil, err
	}

	// keeping this temporarily so that the tests pass on each commit of the branch
	err = filepath.WalkDir(path.Join(path.Dir(schemaPath), "extensions"), func(fpath string, d fs.DirEntry, err error) error {
		if !d.IsDir() && filepath.Ext(fpath) == ".json" {
			url, err := getSchemaURL(fpath)
			if err != nil {
				return fmt.Errorf("failed to get schema $id: %w", err)
			}

			f, err := os.Open(fpath)
			if err != nil {
				return fmt.Errorf("could not open extension spec: %v", err)
			}
			defer f.Close()

			if err := c.AddResource(url, f); err != nil {
				return err
			}
			log.Printf("Registered ext schema %v -> %v\n", url, fpath)
		}

		return nil
	})
	if err != nil {
		return nil, err
	}

	mainURL, err := getSchemaURL(schemaPath)
	if err != nil {
		return nil, fmt.Errorf("failed to get $id for root schema: %w", err)
	}

	s, err := c.Compile(mainURL)
	if err != nil {
		return nil, err
	}

	return &SchemaValidator{s}, nil
}

func (sv *SchemaValidator) ValidateBytes(input []byte) error {
	var obj interface{}
	if err := json.Unmarshal(input, &obj); err != nil {
		return fmt.Errorf("unable to unmarshal json: %v", err)
	}

	return sv.s.Validate(obj)
}

func (sv *SchemaValidator) ValidateFile(filename string) error {
	data, err := ioutil.ReadFile(filename)
	if err != nil {
		return fmt.Errorf("cannot open input file: %v", err)
	}

	return sv.ValidateBytes(data)
}
