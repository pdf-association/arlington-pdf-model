/*
 * XMLCreator.java
 * Copyright 2020 PDF Association, Inc. https://www.pdfa.org
 *
 * This material is based upon work supported by the Defense Advanced
 * Research Projects Agency (DARPA) under Contract No. HR001119C0079.
 * Any opinions, findings and conclusions or recommendations expressed
 * in this material are those of the author(s) and do not necessarily
 * reflect the views of the Defense Advanced Research Projects Agency
 * (DARPA). Approved for public release.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Contributors: Roman Toda, Frantisek Forgac, Normex
 */
package gcxml;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.FactoryConfigurationError;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Result;
import javax.xml.transform.Source;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

/**
 * A class to create XML equivalent versions of an Arlington TSV file set.
 */
public class XMLCreator {
    /**
     * Input folder where Arlington TSV files are.
     * Defaults to './tsv/latest' on the assumption running in the Arlington
     * main folder.
     */
    private String input_folder;

    /**
     * Output folder where XML files will be written.
     * Defaults to './xml/' on the assumption running in the Arlington
     * main folder.
     */
    private String output_folder;

    /**
     * TSVHandler object that can use to reduce TSV fields, based on
     * Arlington "Type" field PDF version predicates.
     */
    private TSVHandler tsv = null;

    /**
     * the PDF Version of XML being created
     */
    private double pdf_ver = 0;

    /**
     * Alphabetically sorted array of File objects that
     * comprise an Arlington TSV file set.
     */
    private File[] list_of_files = null;

    /**
     * TSV delimiter - should be TAB
     */
    private char delimiter = '\t';

    /**
     * Current Arlington TSV filename (object) being processed.
     * Used for error and assertion messages during XML creation.
     */
    private String current_entry;

    /**
     * XML document root object to which we will add XML elements
     */
    private DocumentBuilderFactory dom_factory = null;
    private DocumentBuilder dom_builder = null;
    private Document new_doc = null;

    /**
     * Converts the Arlington PDF Model from a TSV file set to a single
     * monolithic XML representation.
     *
     * @param list_of_files  array of Arlington TSV file names
     * @param delimiter    should be '\t'
     */
    public XMLCreator(File[] list_of_files, char delimiter) throws Exception {
        this.output_folder = System.getProperty("user.dir") + "/xml/";
        this.input_folder  = System.getProperty("user.dir") + "/tsv/latest/";

        // sort Arlington TSV files by name alphabetically
        ArrayList<File> arr_file = new ArrayList<>();
        for (File file : list_of_files) {
            arr_file.add(file);
        }
        arr_file.sort((p1, p2) -> p1.compareTo(p2));

        this.list_of_files = new File[arr_file.size()];
        for (int i = 0; i < arr_file.size(); i++) {
            this.list_of_files[i] = arr_file.get(i);
        }

        this.delimiter = delimiter;
        this.current_entry = "";
        
        dom_factory = DocumentBuilderFactory.newInstance();
        dom_builder = dom_factory.newDocumentBuilder();
        new_doc = dom_builder.newDocument();        
    }

    /**
     * Creates a specific XML file for a specific PDF version based on
     * an Arlington TSV file set.
     *
     * @param pdf_version  the PDF version (as a string)
     */
    public void createXML(String pdf_version) {
        output_folder += "pdf_grammar" + pdf_version + ".xml" ;
        tsv = new TSVHandler();
        pdf_ver = Float.parseFloat(pdf_version);

        int object_count = 0;
        try {
            // Root element
            Element root_elem = new_doc.createElement("PDF");
            root_elem.setAttribute("pdf_version", pdf_version);
            root_elem.setAttribute("grammar_version", Gcxml.Gcxml_version);
            root_elem.setAttribute("iso_ref", "ISO 32000-2:2020");
            new_doc.appendChild(root_elem);

            // Read each Arlington TSV file
            for (File file : list_of_files) {
                if (file.isFile() && file.canRead() && file.exists()) {
                    BufferedReader tsv_reader;
                    String temp = input_folder;
                    String file_name = file.getName().substring(0, file.getName().length()-4); // no file extension ".tsv"
                    temp += file_name + ".tsv";
                    tsv_reader = new BufferedReader(new FileReader(temp));
                    System.out.println("Processing " + file_name + " for PDF " + pdf_version);

                    // remove TSV header row
                    String current_line = tsv_reader.readLine();

                    Element object_elem = new_doc.createElement("OBJECT");
                    object_elem.setAttribute("id", file_name);
                    object_elem.setAttribute("object_number", String.format("%03d",object_count));

                    boolean object_is_array = file_name.contains("Array") || file_name.contains("ColorSpace");

                    while ((current_line = tsv_reader.readLine()) != null) {
                        String[] column_values = current_line.split(Character.toString(delimiter), -1);
                        assert (column_values.length == 12) : "Less than 12 TSV columns!";

                        // set instance varaibles for reporting purposes
                        current_entry = column_values[0];
                        float current_entry_version = reduceSinceVersion(column_values[2]);

                        if (column_values[0].matches("^[0-9]+(\\*)?(?![a-zA-Z\\\\*])")) {
                            object_is_array = true;
                        }

                        // <ENTRY> node: represents single key/array element in the object
                        Element entry_elem = new_doc.createElement("ENTRY");
                        if (current_entry_version <= pdf_ver) {
                            System.out.println("\tKept key: " + current_entry);

                            TSVHandler.TypeListModifier types_reduced = tsv.reduceTypesForVersion(column_values[1], pdf_ver);
                            column_values[1] = types_reduced.getReducedTypes();
                            if (types_reduced.somethingReduced()) {
                                // At least one type got reduced so need to
                                // reduce various other TSV fields accordingly
                                // BEFORE they themselves are reduced
                                column_values[5]  = types_reduced.reduceCorresponding(column_values[5]);  // IndirectReference
                                column_values[7]  = types_reduced.reduceCorresponding(column_values[7]);  // DefaultValue
                                column_values[9]  = types_reduced.reduceCorresponding(column_values[9]);  // SpecialCase
                            }
                            column_values[4]  = tsv.reduceRequiredForVersion(column_values[4], pdf_ver); // Required
                            column_values[8]  = tsv.reduceComplexForVersion(column_values[8], pdf_ver); // PossibleValues
                            column_values[10] = tsv.reduceComplexForVersion(column_values[10], pdf_ver); // Links

                            // <NAME> node: name of the key
                            Element name_elem = nodeName(column_values[0]);
                            assert (name_elem != null) : "Node element was null!";
                            Element introduced_elem = nodeIntroduced(column_values[2]);
                            assert (introduced_elem != null) : "Introduced element was null!";
                            Element deprecated_elem = nodeDeprecated(column_values[3]);
                            Element required_elem = nodeRequired(column_values[4]);
                            assert (required_elem != null) : "Required element was null!";
                            Element indirect_reference_elem = nodeIndirectReference(column_values[1], column_values[5]);
                            assert (indirect_reference_elem != null) : "IndirectReference element was null!";
                            Element inheritable = nodeInheritable(column_values[6]);
                            assert (inheritable != null) : "Inheritable element was null!";
                            Element special_case_elem = nodeSpecialCase(column_values[9]);

                            // <VALUE> node: possible values that can be used for the entry
                            // - colValues[1]: type
                            // - colValues[10]: links
                            // - colValues[6], colValues[7], colValues[8]: other values (optional)
                            Element value_elem = nodeValues(column_values[1], column_values[7], column_values[8], column_values[10]);

                            //append elements to entry. Some are optional.
                            entry_elem.appendChild(name_elem);
                            if (value_elem != null)
                                entry_elem.appendChild(value_elem);
                            entry_elem.appendChild(required_elem);
                            entry_elem.appendChild(indirect_reference_elem);
                            entry_elem.appendChild(inheritable);
                            entry_elem.appendChild(introduced_elem);
                            if (deprecated_elem != null)
                                entry_elem.appendChild(deprecated_elem);
                            if (special_case_elem != null)
                                entry_elem.appendChild(special_case_elem);

                            // append elements to object
                            object_elem.appendChild(entry_elem);
                        }
                        else {
                            System.out.println("\tDropped key: " + current_entry);
                        }
                    } // while row in TSV

                    // append object to root - if there was anyting
                    if (object_elem.hasChildNodes()) {
                        if (object_is_array)
                            object_elem.setAttribute("isArray", "true");
                        System.out.println("\tAdded to XML for PDF " + pdf_version);
                        object_count++;
                        root_elem.appendChild(object_elem);
                    }
                    tsv_reader.close();
                }
            }

            // Save the XML document
            TransformerFactory tran_factory = TransformerFactory.newInstance();
            Transformer transformer = tran_factory.newTransformer();
            transformer.setOutputProperty(OutputKeys.INDENT, "yes");
            transformer.setOutputProperty(OutputKeys.METHOD, "xml");
            transformer.setOutputProperty("{http://xml.apache.org/xslt}indent-amount", "3");
            Source src = new DOMSource(new_doc);
            Result result = new StreamResult(new File(output_folder));
            transformer.transform(src, result);
            System.out.println("Wrote XML for PDF " + pdf_version + " with " + object_count + " objects to " + output_folder);
        }
        catch (Exception exp) {
            System.err.println(exp.toString());
        }
    }

    /**
     * Creates an XML "NAME" element representing the key name or array index.
     *
     * @param col_value  the key name or array index from TSV "Key" field
     * (column 1, never blank)
     *
     * @return a valid Element (always)
     */
    private Element nodeName(String col_value) {
        Element temp_elem = new_doc.createElement("NAME");
        temp_elem.appendChild(new_doc.createTextNode(col_value));
        if (col_value.contains("*"))
            temp_elem.setAttribute("isWildcard", "true");
        return temp_elem;
    }

    /**
     * Creates an XML "INTRODUCED" element representing the PDF version when
     * the current key/array element was introduced.
     *
     * @param col_value  the TSV "SinceVersion" field (column 3, never blank)
     *
     * @return a valid Element (always)
     */
    private Element nodeIntroduced(String col_value) {
        Element temp_elem = new_doc.createElement("INTRODUCED");
        temp_elem.appendChild(new_doc.createTextNode(col_value));
        return temp_elem;
    }

    /**
     * Creates an XML "DEPRECATED" element representing the PDF version when
     * the current key/array element was deprecated.
     *
     * @param col_value  the TSV "DeprecatedIn" field (column 4). Can be empty.
     *
     * @return a valid Element or null
     */
    private Element nodeDeprecated(String col_value) {
        Element temp_elem = null;

        if (!col_value.isBlank()) {
            temp_elem = new_doc.createElement("DEPRECATED");
            temp_elem.appendChild(new_doc.createTextNode(col_value));
        }
        return temp_elem;
    }


    /**
     * Creates an XML "REQUIRED" element representing the required/optional-ness
     * of the current key/array index. Converted to XML "true"/"false" with
     * predicates remaining unchanged.
     *
     * @param col_value  the TSV "Required" field which can be TRUE, FALSE or a
     * predicate. Column 5.
     *
     * @return a valid Element (always)
     */
    private Element nodeRequired(String col_value) {
        Element temp_elem = new_doc.createElement("REQUIRED");
        if (!col_value.startsWith("fn:")) {
            col_value = col_value.toLowerCase();
        }
        temp_elem.appendChild(new_doc.createTextNode(col_value));
        return temp_elem;
    }

    /**
     * Creates an XML "INDIRECT_REFERENCE" element representing whether the
     * current key/array index is required to be direct, indirect or either.
     *
     * @param types   the TSV "Types" string which may be multi-typed
     * @param col_value  the TSV "IndirectReference" field which can be complex,
     *  FALSE, TRUE or a predicate. Column 6.
     *
     * @return a valid Element with children (always)
     */
    private Element nodeIndirectReference(String types, String col_value) {
        Element temp_elem = new_doc.createElement("INDIRECT_REFERENCE");
        Element value;

        String type_arr[] = types.split(";");
        String ir_arr[];

        if (col_value.contains(";")) {
            ir_arr= col_value.split(";");
        }
        else {
            ir_arr = new String[type_arr.length];
            for (int i = 0; i < ir_arr.length; i++)
                ir_arr[i] = col_value;
        }
        assert (ir_arr.length == type_arr.length) : "Mismatched Type and IndirectRef arrays!";

        for (int i = 0; i < type_arr.length; i++) {
            if (ir_arr[i].charAt(0) == '[') {
                // strip [ and ]
                ir_arr[i] = ir_arr[i].substring(1, ir_arr[i].length()-1);
            }

            if ((ir_arr[i].equals("TRUE")) || (ir_arr[i].equals("FALSE"))) {
                value = createNodeValue(type_arr[i], ir_arr[i].toLowerCase());
            }
            else {
                value = createNodeValue(type_arr[i], ir_arr[i]);
            }
            temp_elem.appendChild(value);
        }
        return temp_elem;
    }


    /**
     * Creates an XML "INHERITABLE" element .
     *
     * @param col_value  the TSV "Inheritable" field which can only be TRUE or
     * FALSE. Column 7. Converted to XML "true"/"false".
     *
     * @return null on error, or a valid Element
     */
    private Element nodeInheritable(String col_value) {
        Element temp_elem = new_doc.createElement("INHERITABLE");
        temp_elem.appendChild(new_doc.createTextNode(col_value.toLowerCase()));
        return temp_elem;
    }

    /**
     * Creates an XML "DEFAULT_VALUE" element .
     *
     * @param col_value  the TSV "DefaultValue" field. Column 8. Can be almost
     * anything.
     *
     * @return a valid Element (always)
     */
    private Element nodeDefaultValue(String col_value) {
        Element temp_elem = new_doc.createElement("DEFAULT_VALUE");
        temp_elem.appendChild(new_doc.createTextNode(col_value));
        return temp_elem;
    }


    /**
     * Creates an XML "SPECIAL_CASE" element .
     *
     * @param col_value  the TSV "SpecialCase" field which can be anything.
     * Column 10.
     *
     * @return a valid Element or null
     */
    private Element nodeSpecialCase(String col_value) {
        Element temp_elem = null;
        if (!col_value.isBlank()) {
            temp_elem = new_doc.createElement("SPECIAL_CASE");
            col_value = col_value.substring(1, col_value.length()-1); // strip [ and ]
            temp_elem.appendChild(new_doc.createTextNode(col_value));
        }
        return temp_elem;
    }


    /**
     * Converts "DefaultValue" and "PossibleValues" into XML for a set of Types
     * and Links.
     *
     * @param type an Arlington Type field, possibly complex
     * @param default_value  Arlington "DefaultValue" field, possibly complex
     * @param possible_values Arlington "PossibleValues" field, possibly complex
     * @param links Arlington Links corresponding
     *
     * @return a valid XML Element
     */
    private Element nodeValues(String type, String default_value, String possible_values, String links) {
        Element values_elem = new_doc.createElement("VALUES");

        String[] types = type.split(";");
        String[] arr_links = links.split(";");
        assert (types.length == arr_links.length) : "Types and Links are different lengths!";
        assert (!type.contains("fn:")) : "Types contained a predicate!";

        String[] pos_values = possible_values.split(";");
        String[] dft_values = default_value.split(";");
        assert (pos_values.length == arr_links.length) : "PossibleValues and Links are not the same length!";
        assert (dft_values.length == arr_links.length) : "DefaultValue and Links are not the same length!";

        for (int i = 0; i < types.length; i++) {
            Element value;
            String t = types[i];

            // Is it an Arlington type needing a Link?
            if ("array".equals(t) ||
                "dictionary".equals(t) ||
                "stream".equals(t) ||
                ("number-tree".equals(t) && !links.isBlank()) ||
                ("name-tree".equals(t) && !links.isBlank())) {
                // Strip [ and ]
                assert (arr_links[i].charAt(0) == '[') : "No opening [ on Links";
                assert (arr_links[i].charAt(arr_links[i].length()-1) == ']') : "No closing ] on Links";
                String a = arr_links[i].substring(1, arr_links[i].length() - 1);

                // COMMAs are ambiguous: separators or inside predicates?
                while (!a.isBlank()) {
                    if (a.startsWith("fn:")) {
                        // get up to closing bracket )
                        int j = tsv.indexOfOuterCloseBracket(a);
                        assert j != -1: "No ')' for predicate!";

                        // Get encapsulating predicate incl. close bracket
                        String s = a.substring(0, j + 1);
                        value = createNodeValue(t, s);
                        values_elem.appendChild(value);

                        if (j + 2 < a.length()) {
                            a = a.substring(j + 2, a.length());
                        }
                        else {
                            a = "";
                        }

                        // remove COMMA if not at end of string
                        if ((!a.isBlank()) && (a.charAt(0) == ',')) {
                            a = a.substring(1, a.length());
                        }
                    }
                    else {
                        String s = a;
                        if (a.indexOf(',') > 0) {
                            s = a.substring(0, a.indexOf(','));
                            a = a.substring(a.indexOf(',') + 1, a.length());
                        }
                        else {
                            a = "";
                        }
                        assert (!s.isBlank()) : "Adding empty value!";
                        value = createNodeValue(t, s);
                        values_elem.appendChild(value);
                    }
                } // while
            } // if Linkable-type

            // any PossibleValues?
            if ((i < pos_values.length) && (!pos_values[i].isBlank())) {
                // Strip [ and ]
                assert (pos_values[i].charAt(0) == '[') : "No opening [ on PossibleValue";
                assert (pos_values[i].charAt(pos_values[i].length()-1) == ']') : "No closing ] on PossibleValue";
                String a = pos_values[i].substring(1, pos_values[i].length() - 1);

                // COMMAs are ambiguous: separators or inside predicates?
                while (!a.isBlank()) {
                    if (a.startsWith("fn:")) {
                        // get up to closing bracket )
                        int j = tsv.indexOfOuterCloseBracket(a);
                        assert j != -1: "No ')' for predicate!";

                        // Get encapsulating predicate incl. close bracket
                        String s = a.substring(0, j + 1);
                        value = createNodeValue(t, s);
                        values_elem.appendChild(value);

                        if (j + 2 < a.length()) {
                            a = a.substring(j + 2, a.length());
                        }
                        else {
                            a = "";
                        }

                        // remove COMMA if not at end of string
                        if ((!a.isBlank()) && (a.charAt(0) == ',')) {
                            a = a.substring(1, a.length());
                        }
                    }
                    else {
                        String s = a;
                        if (a.indexOf(',') >= 0) {
                            s = a.substring(0, a.indexOf(','));
                            a = a.substring(a.indexOf(',') + 1, a.length());
                        }
                        else {
                            a = "";
                        }
                        value = createNodeValue(t, s);
                        values_elem.appendChild(value);                        }
                } // while
            } // if PossibleValues

            // any DefaultValue?
            if ((i < dft_values.length) && (!dft_values[i].isBlank())) {
                value = createNodeValue(t, dft_values[i]);
                value.setAttribute("isDefaultValue", "true");
                values_elem.appendChild(value);
            }
        } // for-each type

        if (values_elem.hasChildNodes())
            return values_elem;
        else
            return null;
    }

   /**
     * Creates a single "VALUE" element for a Type t with value
     *
     * @param t      an Arlington type (just one)
     * @param value  the value for the VALUE node
     * @return       a new XML VALUE element
     */
    private Element createNodeValue(String t, String value) {
        Element valueElem = new_doc.createElement("VALUE");
        assert (!t.contains(";")) : "VALUE node type had a SEMI-COLON!";
        valueElem.setAttribute("type", t);
        valueElem.appendChild(new_doc.createTextNode(value));
        return valueElem;
    }
    
    /**
     * Processes an Arlington "SinceVersion" field that might contain version
     * predicates and reduces it appropriately for the specified PDF version.
     * Examples include:
     * - fn:Extension(XYZ)
     * - fn:Extension(XYZ,1.3)
     * - fn:Eval(fn:Extension(XYZ,1.5) || 2.0)
     *
     * @param sincever  the SinceVersion field from an Arlington TSV file

     * @return the lowest PDF version ("1.0", "1.1", etc)
     */
    public float reduceSinceVersion(String sincever) {
        if (sincever.startsWith("fn:")) {
            if (sincever.matches("fn:Extension\\([A-Za-z0-9_]+\\)")) {
                // Predicate: fn:Extension(AAA) - all versions of PDF
                return (float) 1.0;
            }
            else {
                // Predicate: fn:Extension(AAA,x.y) - only since x.y
                Pattern p1 = Pattern.compile("fn:Extension\\([A-Za-z0-9_]+\\,([12]\\.[0-7])\\)");
                Matcher m1 = p1.matcher(sincever);
                if (m1.matches()) {
                    return Float.parseFloat(m1.group(1));
                }
                Pattern p2 = Pattern.compile("fn:Eval\\(fn:Extension\\([A-Za-z0-9_]+\\,([12]\\.[0-7])\\) \\|\\| ([12]\\.[0-7])\\)");
                Matcher m2 = p2.matcher(sincever);
                if (m2.matches()) {
                    return Float.parseFloat(m2.group(1));
                }
            }
        }
        else {
            // Just a normal PDF version
            return Float.parseFloat(sincever);
        }
        return (float) 1.0; // Assume every PDF version
    }
}
