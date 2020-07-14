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
 *
 * @author fero
 */
public class XMLCreator {
    private String pdf_version = "";
    private final String grammar_version = Gcxml.grammar_version;

    private String input_folder;
    private String output_folder;
    
    private File[] list_of_files = null;
    private String delimiter = "";
    
    private String current_entry;
    private int error_count;
    private float current_entry_version = 0;
    
    private DocumentBuilderFactory dom_factory = null;
    private DocumentBuilder dom_builder = null;
    private Document new_doc = null;
    
    public XMLCreator(File[] list_of_files, String delimiter, String pdf_version) {
        this.output_folder = System.getProperty("user.dir") + "/xml/";
        this.input_folder = System.getProperty("user.dir") + "/tsv/latest/";
        
        // sort files by name alphabetically
        ArrayList<File> arr_file = new ArrayList<>();
        for(File file : list_of_files){
            arr_file.add(file);
        }
        arr_file.sort((p1, p2) -> p1.compareTo(p2));
        this.list_of_files = new File[arr_file.size()];
        for(int i=0; i < arr_file.size(); i++){
            this.list_of_files[i] = arr_file.get(i);
        }
        
        this.delimiter = delimiter;
        this.current_entry = "";
        this.error_count = 0;
        this.pdf_version = pdf_version;
        
        try {
            dom_factory = DocumentBuilderFactory.newInstance();
            dom_builder = dom_factory.newDocumentBuilder();
            new_doc = dom_builder.newDocument();
        } catch (FactoryConfigurationError exp) {
            System.err.println(exp.toString());
        } catch (ParserConfigurationException exp) {
            System.err.println(exp.toString());
        } catch (Exception exp) {
            System.err.println(exp.toString());
        }
    }
    
    public void convertFile() {
        output_folder += "pdf_grammar" + pdf_version + ".xml" ;
        
        int rows_count = -1;
        int object_count = 0;
        try {
            // Root element
            Element root_elem = new_doc.createElement("PDF");
            root_elem.setAttribute("pdf_version", pdf_version );
            root_elem.setAttribute("grammar_version", grammar_version );
            root_elem.setAttribute("iso_ref", "ISO-32000" );
            //root_elem.setAttribute("xmlns:xlink", "http://www.w3.org/1999/xlink");
            new_doc.appendChild(root_elem);
            
            // Read tsv files
            for (File file : list_of_files) {
                if (file.isFile() && file.canRead() && file.exists()) {
                    error_count = 0;
                    BufferedReader tsv_reader;
                    String temp = input_folder;
                    String file_name = file.getName().substring(0, file.getName().length()-4);
                    temp += file_name + ".tsv";
                    tsv_reader = new BufferedReader(new FileReader(temp));
                    String[] column_headers = null;
                    System.out.println("Processing " +file_name);
                    // removes header row
                    String current_line = tsv_reader.readLine();
                    if (current_line != null) {
                        column_headers = current_line.split(delimiter, -1);
                        for (int i = 0; i < column_headers.length; i++){
                            column_headers[i] = column_headers[i].toUpperCase();
                        }
                    }
                    
                    // FUTURE WORK : add/change attributes
                    Element object_elem = new_doc.createElement("OBJECT");
                    object_elem.setAttribute("id", file_name);
                    object_elem.setAttribute("object_number", String.format("%03d",object_count));
                    while ((current_line = tsv_reader.readLine()) != null) {
                        // creates <ENTRY> node -> represents single key/entry in the object
                        Element entry_elem = new_doc.createElement("ENTRY");
                        String[] column_values = current_line.split(delimiter,-1);
                        current_entry = column_values[0];
                        current_entry_version = Float.parseFloat(column_values[2]);
                        if(current_entry_version<=Float.parseFloat(pdf_version)){
                            // creates <NAME> node -> name of the key
                            Element name_elem = nodeName(column_values[0]);
                            
                            // creates <VALUE> node -> possible values that can be used for the entry
                            // colValues[1] -> type
                            // colValues[10] -> link(csv) VALIDATE(xml)
                            // colValues[6], colValues[7], colValues[8] -> other values (optional)
                            Element value_elem = null;
                            String types = column_values[1];
                            String[] arr_types = types.split(";", -1);
                            boolean is_linkable = false;
                            for(int i = 0; i < arr_types.length; i++){
                                if("dictionary".equals(arr_types[i]) || "array".equals(arr_types[i]) ||"stream".equals(arr_types[i])){
                                    is_linkable = true;
                                }
                            }
                            
                            if(is_linkable == true){
                                value_elem = nodeValuesLinkable(column_values[1], column_values[10]);
                            }else{
                                value_elem = nodeValues(column_values[1], column_values[7], column_values[8]);
                            }
                            // creates <INTRODUCED>, <DEPRECATED>, <REQUIRED>, <INDIRECTREFERENCE>
                            Element introduced_elem = nodeIntroduced(column_values[2]);
                            Element deprecated_elem = nodeDeprecated(column_values[3]);
                            Element required_elem = nodeRequired(column_values[4]);
                            Element indirect_reference_elem = nodeIndirectReference(column_values[5]);
                            Element inheritable = nodeInheritable(column_values[6]);
                            Element special_case_elem = nodeSpecialCase(column_values[9]);
                            
                            if((name_elem != null) && (value_elem != null) && (introduced_elem != null) &&
                                    (deprecated_elem != null) && (required_elem != null) && (indirect_reference_elem != null) &&
                                    (special_case_elem != null)){
                                //append elements to entry
                                entry_elem.appendChild(name_elem);
                                entry_elem.appendChild(value_elem);
                                entry_elem.appendChild(required_elem);
                                entry_elem.appendChild(indirect_reference_elem);
                                entry_elem.appendChild(inheritable);
                                entry_elem.appendChild(introduced_elem);
                                entry_elem.appendChild(deprecated_elem);
                                entry_elem.appendChild(special_case_elem);
                                // append elements to object
                                object_elem.appendChild(entry_elem);
                            }
                        }
                    }
                    // append object to root
                    if(object_elem.hasChildNodes()){
                        object_count++;
                        root_elem.appendChild(object_elem);
                    }
                    if(error_count == 0){
                        System.out.println("Finished succesfully.");
                    }else{
                        System.out.println("Processing failed! " +error_count+" errors were encountered while processing object.");
                    }
                    tsv_reader.close();                   
                }
            }
            // Save the document to the disk file
            // properties setup
            TransformerFactory tran_factory = TransformerFactory.newInstance();
            Transformer transformer = tran_factory.newTransformer();
            transformer.setOutputProperty(OutputKeys.INDENT, "yes");
            transformer.setOutputProperty(OutputKeys.METHOD, "xml");
            transformer.setOutputProperty("{http://xml.apache.org/xslt}indent-amount", "3");
            Source src = new DOMSource(new_doc);
            Result result = new StreamResult(new File(output_folder));
            transformer.transform(src, result);
        } catch (IOException exp) {
            System.err.println(exp.toString());
        } catch (Exception exp) {
            System.err.println(exp.toString());
        }
    }
    
    private Element nodeName(String col_value) {
        Element temp_elem = null;
        if(!col_value.isBlank()){
            temp_elem = new_doc.createElement("NAME");
            temp_elem.appendChild(new_doc.createTextNode(col_value));
        }else{
            System.out.println("\tERROR. While processing entry: " +current_entry+ ". Failed to create NAME node. Missing value for key name.");
            ++error_count;
        }
        return temp_elem;
    }
    
    private Element nodeIntroduced(String col_value) {
        Element temp_elem = null;
        if(!col_value.isBlank()){
            temp_elem = new_doc.createElement("INTRODUCED");
            temp_elem.appendChild(new_doc.createTextNode(col_value));
        }else{
            System.out.println("\tERROR. While processing entry: " +current_entry+ ". Failed to create SINCEVERSION node. Missing value for since version.");
            ++error_count;
        }
        return temp_elem;
    }
    
    private Element nodeDeprecated(String col_value) {
        Element temp_elem = new_doc.createElement("DEPRECATED");
        temp_elem.appendChild(new_doc.createTextNode(col_value));
        return temp_elem;
    }
    
    private Element nodeRequired(String col_value) {
        Element temp_elem = null;
        if(!col_value.isBlank()){
            temp_elem = new_doc.createElement("REQUIRED");
            temp_elem.appendChild(new_doc.createTextNode(col_value.toLowerCase()));
        }else{
            System.out.println("\tERROR. While processing entry: " +current_entry+ ". Failed to create REQUIRED node. Missing value for required. Shall be TRUE or FALSE.");
            ++error_count;
        }
        return temp_elem;
    }
    
    private Element nodeIndirectReference(String col_value) {
        Element temp_elem = null;
        if(!col_value.isBlank()){
            temp_elem = new_doc.createElement("INDIRECT_REFERENCE");
            temp_elem.appendChild(new_doc.createTextNode(col_value.toLowerCase()));
        }else{
            System.out.println("\tERROR. While processing entry: " +current_entry+ ". Failed to create INDIRECTREFERENCE node. Missing value for indirect reference. Shall be TRUE or FALSE.");
            ++error_count;
        }
        return temp_elem;
    }
    
    private Element nodeValuesLinkable(String type, String validate) {
        Element values_elem = new_doc.createElement("VALUES");
        
        validate = validate.replace("[", "");
        validate = validate.replace("]", "");
        
        String[] types = null;
        String[] values = null;
        
        types = type.split(";", -1);
        values = validate.split(";", -1);
        
        
        if(types.length == values.length){
            for(int i = 0; i < types.length; i++){
                int k = 0;
                String[] temp = values[i].split(",",-1);
                for(int j = 0; j < temp.length; j++){
                    Element value_elem = new_doc.createElement("VALUE");
                    value_elem.setAttribute("type", types[i]);
                    if("dictionary".equals(types[i]) || "array".equals(types[i]) || "stream".equals(types[i])){
                        String node_value = temp[j];
                        //value_elem.setAttribute("xlink:type", "simple");
                        //value_elem.setAttribute("xlink:href","_pdf_grammar2.0.xml#xpointer(id(" +node_value+ "))");
                        value_elem.appendChild(new_doc.createTextNode(node_value));
                        if(node_value.isBlank()){
                            System.out.println("\tWARNING. Missing value in entry: "+current_entry+ ". VALUE node was created but has no value.");
                        }
                    }
                    values_elem.appendChild(value_elem);
                    k++;
                }
            }
        }else{
            System.out.println("\tERROR. While processing entry: " +current_entry+ ". Failed to create VALUES node. Types and links do not match.");
            ++error_count;
        }
        return values_elem;
    }
    
    private Element nodeValues(String type, String default_value, String possible_values) {
        Element values_elem = new_doc.createElement("VALUES");
        
        String[] types = null;
        String[] pos_values = null;
        
        possible_values = possible_values.replace("[", "");
        possible_values = possible_values.replace("]", "");
        
        types = type.split(";", -1);
        
        if(!possible_values.isBlank()){
            pos_values = possible_values.split(";", -1);
            for(int i = 0; i < types.length; i++){
                int k = 0;
                String[] temp = pos_values[i].split(",",-1);
                for(int j = 0; j < temp.length; j++){
                    Element valueElem = new_doc.createElement("VALUE");
                    valueElem.setAttribute("type", types[i]);
                    if(!temp[j].isEmpty()){
                        valueElem.appendChild(new_doc.createTextNode(temp[j]));  
                    }
                    values_elem.appendChild(valueElem);
                }
            }
        }else{
            for(int i = 0; i < types.length; i++){
                int k = 0;
                Element value_elem = new_doc.createElement("VALUE");
                value_elem.setAttribute("type", types[i]);
                values_elem.appendChild(value_elem);
            }
        }
        if(!default_value.isBlank()){
            Element default_elem = new_doc.createElement("DEFAULT_VALUE");
            default_elem.appendChild(new_doc.createTextNode(default_value));
            values_elem.appendChild(default_elem);
        }
        return values_elem;
    }

    private Element nodeSpecialCase(String col_value) {
        Element temp_elem = new_doc.createElement("SPECIAL_CASE");
        temp_elem.appendChild(new_doc.createTextNode(col_value));
        return temp_elem;
    }

    private Element nodeInheritable(String col_value) {
        Element temp_elem = null;
        if(!col_value.isBlank()){
            temp_elem = new_doc.createElement("INHERITABLE");
            temp_elem.appendChild(new_doc.createTextNode(col_value.toLowerCase()));
        }else{
            System.out.println("\tERROR. While processing entry: " +current_entry+ ". Failed to create INHERITABLE node. Missing value for inheritable. Shall be TRUE or FALSE.");
            ++error_count;
        }
        return temp_elem;
    }
}
