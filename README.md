CONTENTS OF THIS FILE
---------------------

 * summary
 * requirements
 * installation
 * configuration
 * customization
 * contact


SUMMARY
-------

Bulk Newspaper Ingester

Handles iArchives XML and old Olive XML.

### iArchives XML
Scans for unit.xml files, and attempts to create newspapers from them. Some
assumptions have been made in terms of folder structure: The tiffs are located
in a separate directory at the same level as the "years"...  So something like:

* "target" directory
  * /&lt;one or more sub-directories&gt;
    * unit.xml
    * pages/
      * page-id.xml
    * pdf/
      * page-id.pdf
    * Archival_Images/
      * page-id.tif

### Olive XML
* "target" directory
  * /&lt;one or more sub-directories&gt;
    * /year
      * TOC.xml
      * /1
        * Pg01.xml
    * /Archival_images
      * 023-CNO-1888-05-31-001-Single.tif
    

REQUIREMENTS
------------

Dependent on:
* islandora_batch
* islandora_book_batch
* islandora_large_image

INSTALLATION
------------

Install as any other Drupal module.

CONFIGURATION
-------------

There are a couple relevant parameters...  They are available from the drush
help output for the command. Primarily:
* target: Used to indicate the directory in which to search for unit.xml files
* parent: Used to indicate what collection the newspaper issue objects should
  be made a member of.
* namespace: Used to indicate the namespace for the PIDs generated on ingest.
* xml_type: Used to select the type of XML to use for ingest.


CUSTOMIZATION
-------------

This is built using the Islandora_Batch module so check out the useful
documentation on that [module's page](https://github.com/Islandora/islandora_batch#customization).

The includes directory has files which holds the paper and page classes
for the different types of XML.
To modify the location of the required files or to remove objects
(ie. pdf, jpg) check the batchProcess() class function.

For example, in includes/iArchives.class.inc

```
protected function findTiff() { 
  if ($this->tiffPath === NULL) { 
    // Get the URI of the page.xml.
    $page_uri = $this->objectInfo['.']['page']->uri; 
        
    // Break into components. $components = explode('/', $page_uri);
    // Add in component for TIFF directory.
    // Change "pages" to "Archival_Images".
    array_splice($components, -2, 1, 'Archival_Images');

    // Change the file extension.
    $xml = array_pop($components);
    $components[] = pathinfo($xml, PATHINFO_FILENAME) . '.tif';

    $this->tiffPath = implode('/', $components);
  
  }

  return $this->tiffPath;
}
```

If the $page_uri in the above example is 
/dir1/newspapers/19001002/pages/Pg1.xml
then the tiffPath is set to 
/dir1/newspapers/19001002/Archival_Images/Pg1.tif

If your path is different it can be modified.

You can also create a whole new class, and modify the [drush call](https://github.com/uml-digitalinitiatives/uofm_newspaper_batch/blob/7.x-split-xml/uofm_newspaper_batch.drush.inc#L114-L125) to load
the correct Preprocessor.

I know its not fancy but it works.

CONTACT
-------
  
  * [University of Manitoba Libraries - Digital Initiatives](https://github.com/uml-digitalinitiatives)
  * [Jared Whiklo](https://github.com/whikloj)
