Pre-Release Warning
===================
This library is a fresh port, it's more likely to not work than it is to work. Use at your own risk.

About the Exosite Arduino Library
=================================
This library allows you to quickly and easily connect your Spark project to Exosite's Data Platform in the cloud. See the examples folder for example use.

Note: A free account on exosite portals is required: https://portals.exosite.com

License is BSD 3-Clause, Copyright 2014, Exosite LLC (see LICENSE file)

Interface
=========

Constructor
-----------
```cpp
Exosite(Client *_client);
```

```cpp
Exosite(char *_cik, Client *_client);
```

```cpp
Exosite(String _cik, Client *_client);
```

`_cik`: This is used to hard code the CIK into the device can either be a `char[]` or a `String` type. This parameter can be omitted when using provisioning.

`_client`: This is the interface to what ever network device you're using. On the current Spark Core it will always be a `TcpClient`.

writeRead
---------

```cpp
boolean Exosite::writeRead(char* writeString, char* readString, char** returnString)
```

```cpp
boolean Exosite::writeRead(String writeString, String readString, String &returnString)
```

`writeString`: This sets the values to write to certain datasources. eg. "alias3=value3&alias4=value4"

`readString`: This selects which datasources to read by their alias. eg. "alias1&alias2"

`returnstring`: This is the string returned with the values requested in `readString`. eg. "alias1=value1&alias2=value2"

provision
---------
```cpp
boolean Exosite::provision(char* vendorString, char* modelString, char* snString);
```

`vendorString`: The string that identifies the device vendor name.

`modelString`: The string that identifies the device unique model ID.

`snString`: The string that identifies the device's serial number.
