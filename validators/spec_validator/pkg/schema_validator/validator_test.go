package schemavalidator

import (
	"testing"

	js "github.com/santhosh-tekuri/jsonschema/v5"
	"github.com/stretchr/testify/require"
)

func TestSchemaVersion(t *testing.T) {
	json := `{
		"schemaVersion": {
			"major": 2,
			"minor": 0
		},
		"sequenceNumber": 0,
		"timestamp": "2021-10-19T22:59:20+00:00"
	}`

	err := validateString(t, json)
	require.NoError(t, err)
}

func xTestValidateBytesComplexRunArtifact(t *testing.T) {
	json := `{
		"testRunArtifact": {
			"testRunStart": {
				"name": "Error Monitor",
				"version": "392747070",
				"parameters": {
					"@type": "type.googleapis.com/meltan.error_monitor.Params",
					"pollingIntervalSecs": 1,
					"runtimeSecs": 10,
					"ceccThreshold": {
						"maxCountPerDay": 4000
					},
					"ueccThreshold": {
						"maxCountPerDay": 0
					},
					"dimmNameMap": {},
					"monitors": [
						"PCIE_ERROR_MONITOR"
					],
					"pcicrawlerPath": "",
					"pcieThresholds": []
				},
				"dutInfo": [
					{
						"hostname": "yv3-slot2",
						"hardwareComponents":
						[
							{
								"hardwareInfoId": "0",
								"arena": "",
								"name": "PCIE_NODE:0000:04:00.0",
								"partNumber": "",
								"manufacturer": "",
								"mfgPartNumber": "",
								"partType": "endpoint",
								"componentLocation": {
									"devpath": "",
									"odataId": "",
									"blockpath": "0",
									"serialNumber": ""
								}
							},
							{
								"hardwareInfoId": "1",
								"arena": "",
								"name": "PCIE_NODE:0000:00:1d.0",
								"partNumber": "",
								"manufacturer": "",
								"mfgPartNumber": "",
								"partType": "root_port",
								"componentLocation":
								{
									"devpath": "",
									"odataId": "",
									"blockpath": "1",
									"serialNumber": ""
								}
							}
						],
						"softwareInfos":
						[]
					}
				]
			}
		},
		"sequenceNumber": 1,
		"timestamp": "2021-09-17T09:24:17.113215889Z"
	}`

	err := validateString(t, json)
	require.NoError(t, err)
}

func TestValidateBytesInvalid(t *testing.T) {
	json := `{
		"testRunArtifact": {
			"log": {
				"text": "log text",
				"severity": "INFO"
			}
		},
		"testStepArtifact": {
			"testStepId": "1",
			"log": {
				"text": "log text",
				"severity": "INFO"
			}
		},
		"sequenceNumber": 0,
		"timestamp": "2021-10-19T22:59:20+00:00"
	}`

	err := validateString(t, json)
	var ve *js.ValidationError
	require.ErrorAs(t, err, &ve)
}

func TestRunLogSimple(t *testing.T) {
	json := `{
		"testRunArtifact": {
			"log": {
				"message": "log text",
				"severity": "INFO"
			}
		},
		"sequenceNumber": 0,
		"timestamp": "2021-10-19T22:59:20+00:00"
	}`

	err := validateString(t, json)
	require.NoError(t, err)
}

func TestStepLogSimple(t *testing.T) {
	json := `{
		"testStepArtifact": {
			"testStepId": "1",
			"log": {
				"message": "log text",
				"severity": "INFO"
			}
		},
		"sequenceNumber": 1,
		"timestamp": "2021-10-19T22:59:20+00:00"
	}`

	err := validateString(t, json)
	require.NoError(t, err)
}

func TestRunErrorSimple(t *testing.T) {
	json := `{
		"testRunArtifact": {
			"error": {
				"symptom": "test-symptom"
			}
		},
		"sequenceNumber": 0,
		"timestamp": "2021-10-19T22:59:20+00:00"
	}`

	err := validateString(t, json)
	require.NoError(t, err)
}

func TestStepErrorSimple(t *testing.T) {
	json := `{
		"testStepArtifact": {
			"testStepId": "1",
			"error": {
				"message": "log text",
				"symptom": "test-symptom",
				"softwareInfoIds": [
					"1"
				]
			}
		},
		"sequenceNumber": 1,
		"timestamp": "2021-10-19T22:59:20+00:00"
	}`

	err := validateString(t, json)
	require.NoError(t, err)
}

func TestStepFileSimple(t *testing.T) {
	json := `{
		"testStepArtifact": {
			"testStepId": "1",
			"file": {
				"displayName": "mem_cfg_log",
				"uri": "file:///root/mem_cfg_log",
				"description": "DIMM configuration settings.",
				"contentType": "text/plain",
				"isSnapshot": false
			}
		},
		"sequenceNumber": 1,
		"timestamp": "2021-10-19T22:59:20+00:00"
	}`

	err := validateString(t, json)
	require.NoError(t, err)
}

func TestStepDiagnosisSimple(t *testing.T) {
	json := `{
		"testStepArtifact": {
			"diagnosis": {
				"verdict": "mlc-intranode-bandwidth-pass",
				"type": "PASS",
				"message": "intranode bandwidth within threshold.",
				"hardwareInfoId": "1",
				"subcomponent": {
					"type": "BUS",
					"name": "QPI1",
					"location": "CPU-3-2-3",
					"version": "1",
					"revision": "0"
				}
			},
			"testStepId": "1"
		},
		"sequenceNumber": 1,
		"timestamp": "2022-07-25T02:15:58.296032Z"
	}`

	err := validateString(t, json)
	require.NoError(t, err)
}

func TestStepExtensionSimple(t *testing.T) {
	json := `{
		"testStepArtifact": {
			"extension": {
				"name": "extension example",
				"content": {
					"@type": "ExampleExtension",
					"stringField": "example string",
					"numberField": "42"
				}
			},
			"testStepId": "2321"
		},
		"sequenceNumber": 600,
		"timestamp": "2022-07-26T02:34:06.249683Z"
	}`

	err := validateString(t, json)
	require.NoError(t, err)
}

func TestRunStartSimple(t *testing.T) {
	json := `{
		"testRunArtifact": {
			"testRunStart": {
				"name": "mlc_test",
				"version": "1.0",
				"commandLine": "mlc/mlc --use_default_thresholds=true --data_collection_mode=true",
				"parameters": {
					"max_bandwidth": 7200.0,
					"mode": "fast_mode",
					"data_collection_mode": true,
					"min_bandwidth": 700.0,
					"use_default_thresholds": true
				},
				"dutInfo": {
					"id": "1",
					"hostname": "ocp_lab_0222",
					"platformInfos": [
						{
							"info": "memory_optimized"
						}
					],
					"softwareInfos": [
						{
							"softwareInfoId": "1",
							"computerSystem": "primary_node",
							"softwareType": "FIRMWARE",
							"name": "bmc_firmware",
							"version": "10",
							"revision": "11"
						}
					],
					"hardwareInfos": [
						{
							"hardwareInfoId": "1",
							"computerSystem": "primary_node",
							"manager": "bmc0",
							"name": "primary node",
							"location": "MB/DIMM_A1",
							"odataId": "/redfish/v1/Systems/System.Embedded.1/Memory/DIMMSLOTA1",
							"partNumber": "P03052-091",
							"serialNumber": "HMA2022029281901",
							"manufacturer": "hynix",
							"manufacturerPartNumber": "HMA84GR7AFR4N-VK",
							"partType": "DIMM",
							"version": "1",
							"revision": "2"
						}
					]
				}
			}
		},
		"sequenceNumber": 1,
		"timestamp": "2022-07-25T04:49:25.262947Z"
	}`

	err := validateString(t, json)
	require.NoError(t, err)
}

func TestRunEndSimple(t *testing.T) {
	json := `{
		"testRunArtifact": {
			"testRunEnd": {
				"status": "COMPLETE",
				"result": "PASS"
			}
		},
		"sequenceNumber": 100,
		"timestamp": "2022-07-25T04:49:25.262947Z"
	}`

	err := validateString(t, json)
	require.NoError(t, err)
}

func TestStepStartSimple(t *testing.T) {
	json := `{
		"testStepArtifact": {
			"testStepStart": {
				"name": "intranode-bandwidth-check"
			},
			"testStepId": "22"
		},
		"sequenceNumber": 200,
		"timestamp": "2022-07-25T04:56:42.249367Z"
	}`

	err := validateString(t, json)
	require.NoError(t, err)
}

func TestStepEndSimple(t *testing.T) {
	json := `{
		"testStepArtifact": {
			"testStepEnd": {
				"status": "COMPLETE"
			},
			"testStepId": "22"
		},
		"sequenceNumber": 200,
		"timestamp": "2022-07-25T04:54:41.292679Z"
	}`

	err := validateString(t, json)
	require.NoError(t, err)
}

func validateString(t *testing.T, json string) error {
	const schema string = "../../../../json_spec/output/spec.json"

	sv, err := New(schema)
	require.NoError(t, err)

	err = sv.ValidateBytes([]byte(json))
	if err != nil {
		t.Logf("error in validation: %#v", err)
	}

	return err
}
